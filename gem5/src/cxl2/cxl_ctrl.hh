//created by Quyifei

#ifndef __CXL_CTRL_HH__
#define __CXL_CTRL_HH__

#include "mem/mem_ctrl.hh"
#include "params/CXLCtrl.hh"

namespace gem5
{
    class CXLCtrl : public memory::MemCtrl {
        protected:

        class SwitchPort : public memory::QueuedResponsePort {
            RespPacketQueue queue;
            CXLCtrl& cxlCtrl;

            public:
                SwitchPort(const std::string& name, CXLCtrl& _ctrl);
                
                Tick recvAtomic(PacketPtr pkt) override;
                Tick recvAtomicBackdoor(
                        PacketPtr pkt, MemBackdoorPtr &backdoor) override;

                void recvFunctional(PacketPtr pkt) override;
                void recvMemBackdoorReq(const MemBackdoorReq &req,
                        MemBackdoorPtr &backdoor) override;

                bool recvTimingReq(PacketPtr) override;

                AddrRangeList getAddrRanges() const override;

        };


        SwitchPort switchport;

        // not use traverse, map pkt.addr to 1 or 2 


    };
}

#endif
