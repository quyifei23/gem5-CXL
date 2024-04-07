#include "cxl_wal/wal_journal.hh"
#include "debug/WALJournal.hh"

namespace gem5
{
    WALJournal::handle_t::handle_t(transaction_t *transaction,unsigned int sync)
    : transaction(transaction) {
        h_sync = sync;
    }

    WALJournal::transaction_t::transaction_t(journal_t *journal) : journal(journal) {
        t_state = T_RUNNING;
    }

    WALJournal::journal_t::journal_t() {
        transaction_t transaction(this);
        j_running_transaction = &transaction;
    }

    //在当前事务中开始一个新的原子操作
    handle_t *WALJournal::journal_t::journal_start(unsigned int sync) {
        if(!j_running_transaction) {
            transaction_t transaction(this);
            j_running_transaction = &transaction;
        }
        handle_t handle(&j_running_transaction,sync);
        j_running_transaction.handles.push_back(&handle);
        return &handle;
    }

    //在这里我们修改为：通知JBD2即将修改地址为addr中的数据；
    void WALJournal::journal_t::journal_get_write_access(handle_t *handle,PacketPtr pkt) {
        handle->h_transation->pkts.push_back(pkt);
    }

    //立即将所属的transaction提交。
    void WALJournal::journal_t::journal_flush(transaction_t *transaction) {
        transaction->t_state = T_FLUSH;
        while(!transaction->pkts.empty()) {
            transaction->journal->logs.push_back(transaction->pkts.front());
            transaction->pkts.pop_front();
        }
        transaction->journal->j_running_transaction = NULL;
    }

    //将该handle与所属的transaction断开联系，如果该原子操作是同步的，则立即将所属的transaction提交。最后将该handle删除。
    void WALJournal::journal_t::journal_stop(handle_t *handle) {
        if(handle->h_sync == 1) {
            journal_flush(handle->h_transation);
            return ;
        }
        handle->h_transation->handles.pop_front();
    }

    WALJournal::WALJournal(const WALJournalParams &params) :
        SimObject(params),
        cpuPort(params.name + ".cpu_side_port", this),
        memPort(params.name + ".mem_side_port", this),
        blocked(false)
        { }

    Port & WALJournal::getPort(const std::string &if_name, PortID idx) {
        panic_if(idx != InvalidPortID, "This object doesn't support vector ports");
        
        if(if_name == "mem_side_port") {
            return memPort;
        } else if (if_name == "cpu_side_port") {
            return cpuPort;
        } else {
            return SimObject::getPort(if_name, idx);
        }
    }

    AddrRangeList WALJournal::CPUSidePort::getAddrRanges() const {
        return owner->getAddrRanges();
    }

    void WALJournal::CPUSidePort::recvFunctional(PacketPtr pkt) {
        return owner->handleFunctional(pkt);
    }
                                                                                                                                                                          
    void WALJournal::handleFunctional(PacketPtr pkt) {
        memPort.sendFunctional(pkt);
    }

    AddrRangeList WALJournal::getAddrRanges() const {
        DPRINTF(WALJournal, "Sending new ranges\n");
        // Just use the same ranges as whatever is on the memory side.
        return memPort.getAddrRanges();
    }

    void WALJournal::MemSidePort::recvRangeChange() {
        owner->sendRangeChange();
    }

    void WALJournal::sendRangeChange() {
        cpuPort.sendRangeChange();
    }

    bool WALJournal::CPUSidePort::recvTimingReq(PacketPtr pkt) {
        if(!owner->handleRequest(pkt)) {
            needRetry = true;
            return false;
        } else {
            return true;
        }
    }

    bool WALJournal::handleRequest(PacketPtr pkt) {
        if(blocked) {
            return false;
        }
        DPRINTF(WALJournal, "Got request for addr %#x\n", pkt->getAddr());
        blocked = true;
        memPort.sendPacket(pkt);
        return true;
    }

    void WALJournal::MemSidePort::sendPacket(PacketPtr pkt) {
        panic_if(blockedPacket != nullptr,"should never try to send if blocked!");
        if(!sendTimingReq(pkt)) {
            blockedPacket = pkt;
        }
    }

    void WALJournal::MemSidePort::recvReqRetry() {
        assert(blockedPacket != nullptr);
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
    }

    bool WALJournal::MemSidePort::recvTimingResp(PacketPtr pkt) {
        return owner->handleResponse(pkt);
    }

    bool WALJournal::handleResponse(PacketPtr pkt) {
        assert(blocked);
        DPRINTF(WALJournal, "Got response for addr %#x\n", pkt->getAddr());
        blocked = false;
        cpuPort.sendPacket(pkt);

        cpuPort.trySendRetry();
        return true;
    }

    void WALJournal::CPUSidePort::sendPacket(PacketPtr pkt) {
        panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
        if(!sendTimingResp(pkt)) {
            blockedPacket = pkt;
        }
    }

    void WALJournal::CPUSidePort::recvRespRetry() {
        assert(blockedPacket != nullptr);
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
    }

    void WALJournal::CPUSidePort::trySendRetry()
    {
        if (needRetry && blockedPacket == nullptr) {
            needRetry = false;
            DPRINTF(WALJournal, "Sending retry req for %d\n", id);
            sendRetryReq();
        }
    }

}