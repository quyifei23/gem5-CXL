#ifndef __CXL_SWITCH_HH__
#define __CXL_SWITCH_HH__

#include "mem/port.hh"
#include "params/CXLSwitch.hh"
#include "base/trace.hh"
#include "sim/sim_object.hh"
#include "base/statistics.hh"
#include "sim/clocked_object.hh"
namespace gem5
{

    // class SwitchSidePort : public RequestPort {
    //     private:
    //         CXLSwitch *owner;
    //     public:
    //         SwitchSidePort(const std::string& name, CXLSwitch *owner) :
    //             RequestPort(name,owner),owner(owner)
    //         { }
    //     protected:
    //     bool recvTimingResp(PacketPtr pkt) override;
    //     void recvReqRetry() override;
    //     void recvRangeChange() override;
    // };


    class CXLSwitch : public ClockedObject {

        class CPUSidePort : public ResponsePort {
            private:
                CXLSwitch *owner;
                bool needRetry;
                PacketPtr blockedPacket;
            public:
                CPUSidePort(const std::string& name, CXLSwitch *owner) :
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
                CXLSwitch *owner;
                PacketPtr blockedPacket;
            public:
                MemSidePort(const std::string& name, CXLSwitch *owner) :
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

        CPUSidePort cpuPort;

        MemSidePort memPort;
        MemSidePort switchPort;

        bool blocked;
        const Cycles latency;

        public:
            CXLSwitch(const CXLSwitchParams &params);
            Port &getPort(const std::string &if_name,PortID idx=InvalidPortID) override;

        };
}
#endif