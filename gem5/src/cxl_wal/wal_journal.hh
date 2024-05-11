#ifndef __WAL_JOURNAL_HH__
#define __WAL_JOURNAL_HH__

#include "mem/port.hh"
#include "params/WALJournal.hh"
#include "sim/sim_object.hh"
#include "base/trace.hh"
#include <list>
#include "base/statistics.hh"
#include "sim/clocked_object.hh"

namespace gem5 {
    class WALJournal : public ClockedObject {
        class CPUSidePort : public ResponsePort {
            private:
                WALJournal *owner;
                bool needRetry;
                PacketPtr blockedPacket;
            public:
                CPUSidePort(const std::string& name, WALJournal *owner) :
                    ResponsePort(name,owner) ,owner(owner),needRetry(false),blockedPacket(nullptr)
                { }

                void sendPacket(PacketPtr pkt);
                AddrRangeList getAddrRanges() const override;
                void trySendRetry();
            protected:
                Tick recvAtomic(PacketPtr pkt) override { panic("recvAtomic unimpl."); }
                void recvFunctional(PacketPtr pkt) override;
                bool recvTimingReq(PacketPtr pkt) override;
                void recvRespRetry() override;
        };

        class MemSidePort : public RequestPort {
            private:
                WALJournal *owner;
                PacketPtr blockedPacket;
            public:
                MemSidePort(const std::string& name, WALJournal *owner) :
                    RequestPort(name,owner),owner(owner),blockedPacket(nullptr)
                { }

                void sendPacket(PacketPtr pkt);
                
            protected:
                bool recvTimingResp(PacketPtr pkt) override;
                void recvReqRetry() override;
                void recvRangeChange() override;
        };
        bool handleRequest(PacketPtr pkt);
        bool handleResponse(PacketPtr pkt);
        void handleFunctional(PacketPtr pkt);
        AddrRangeList getAddrRanges() const;
        void sendRangeChange();

        class handle_t;
        class transaction_t;
        class journal_t;
        //add handle
        class handle_t {
            public:
                transaction_t *h_transaction; // 本原子操作属于哪个transaction
                unsigned int h_sync; // 处理完该原子操作以后，立即将所属的transaction提交
                handle_t(transaction_t *transaction,unsigned int sync);
        };
        //add transaction
        enum state {
            T_RUNNING,
            T_LOCKED,
            T_FLUSH,
            T_COMMIT,
            T_COMMIT_RECORD,
            T_FINISHED
        };
        class transaction_t {
            public:
                enum state t_state;
                int capacity;
                std::list<handle_t *> handles;
                std::list<PacketPtr> pkts;
                journal_t *journal;
                transaction_t(journal_t *journal,int capacity);
        };
        //add journal
        class journal_t {
            public:
                std::list<PacketPtr> logs;
                transaction_t *j_running_transaction;

                handle_t *journal_start(unsigned int sync);//在当前事务中开始一个新的原子操作
                void journal_get_write_access(handle_t *handle,PacketPtr pkt);//在这里我们修改为：通知JBD2即将修改地址为addr中的数据；
                void journal_flush(transaction_t *transaction);//立即将所属的transaction提交。
                void journal_stop(handle_t *handle);//将该handle与所属的transaction断开联系，如果该原子操作是同步的，则立即将所属的transaction提交。最后将该handle删除。
                journal_t();
        };

        CPUSidePort cpuPort;

        MemSidePort memPort;

        journal_t journal;
        const Cycles latency;
        bool blocked;

        public:
            WALJournal(const WALJournalParams &params);
            Port &getPort(const std::string &if_name,PortID idx=InvalidPortID) override;


    };
}
#endif