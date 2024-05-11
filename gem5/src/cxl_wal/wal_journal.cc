#include "cxl_wal/wal_journal.hh"
#include "debug/WALJournal.hh"
#include "debug/journal_t.hh"
#include<iostream>
namespace gem5
{
    WALJournal::handle_t::handle_t(transaction_t *transaction,unsigned int sync)
    : h_transaction(transaction) {
        h_sync = sync;
    }

    WALJournal::transaction_t::transaction_t(journal_t *journal,int capacity) : journal(journal),capacity(capacity) {
        t_state = T_RUNNING;
    }

    WALJournal::journal_t::journal_t() {
        transaction_t *transaction = new transaction_t(this,5);
        j_running_transaction = transaction;
    }

    //在当前事务中开始一个新的原子操作
    WALJournal::handle_t *WALJournal::journal_t::journal_start(unsigned int sync) {
        if(!j_running_transaction) {
            transaction_t *transaction = new transaction_t(this,5);
            j_running_transaction = transaction;
        } else if (j_running_transaction->capacity == j_running_transaction->handles.size()) {
            journal_flush(j_running_transaction);
            transaction_t *transaction = new transaction_t(this,5);
            j_running_transaction = transaction;
        }
        handle_t *handle=new handle_t(j_running_transaction,sync);
        j_running_transaction->handles.push_back(handle);
        return handle;
    }

    //在这里我们修改为：通知JBD2即将修改地址为addr中的数据；
    void WALJournal::journal_t::journal_get_write_access(handle_t *handle,PacketPtr pkt) {
        handle->h_transaction->pkts.push_back(pkt);
    }

    //立即将所属的transaction提交。
    void WALJournal::journal_t::journal_flush(transaction_t *transaction) {
        transaction->t_state = T_FLUSH;
        while(!transaction->pkts.empty()) {
            transaction->journal->logs.push_back(transaction->pkts.front());
            transaction->pkts.pop_front();
        }
        transaction->journal->j_running_transaction = NULL;
        std::cout<< "**********log**********" << std::endl;
        // for(std::list<PacketPtr>::iterator i=logs.begin();i!=logs.end();i++) {
        //     DPRINTF(WALJournal, "addr : %#x \n",(*i)->getAddr());
        // }
        delete(transaction);
    }

    //将该handle与所属的transaction断开联系，如果该原子操作是同步的，则立即将所属的transaction提交。最后将该handle删除。
    void WALJournal::journal_t::journal_stop(handle_t *handle) {
        if(handle->h_sync == 1) {
            journal_flush(handle->h_transaction);
            delete(handle);
            return ;
        }
        handle->h_transaction->handles.pop_front();
        delete(handle);
    }

    WALJournal::WALJournal(const WALJournalParams &params) :
        ClockedObject(params),
        cpuPort(params.name + ".cpu_side_port", this),
        memPort(params.name + ".mem_side_port", this),
        latency(params.latency),
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
        //here insert the WAL latency
        schedule(new EventFunctionWrapper([this, pkt]{
        DPRINTF(WALJournal, "Got request for addr %#x\n", pkt->getAddr());
        blocked = true;
        
        //add
        if(pkt->isWrite()) {
            DPRINTF(WALJournal, "get a write request\n");
            handle_t *handle = journal.journal_start(0);
            DPRINTF(WALJournal, "init a new handle\n");
            journal.journal_get_write_access(handle,pkt);
            DPRINTF(WALJournal, "merge into current transation\n");
            journal.journal_stop(handle);
            DPRINTF(WALJournal, "finish the new handle\n");
        }

        memPort.sendPacket(pkt);
        },name() + ".WALEvent", true),clockEdge(latency));
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