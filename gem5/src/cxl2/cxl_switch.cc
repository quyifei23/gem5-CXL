#include "cxl2/cxl_switch.hh"
#include "debug/CXLSwitch.hh"

namespace gem5
{
    CXLSwitch::CXLSwitch(const CXLSwitchParams &params) :
        SimObject(params),
        cpuPort(params.name + ".cpu_side_port", this),
        memPort(params.name + ".mem_side_port", this),
        switchPort(params.name + ".switch_side_port", this),
        blocked(false)
        { }

    Port & CXLSwitch::getPort(const std::string &if_name, PortID idx) {
        panic_if(idx != InvalidPortID, "This object doesn't support vector ports");
        
        if(if_name == "mem_side_port") {
            return memPort;
        } else if (if_name == "cpu_side_port") {
            return cpuPort;
        } else if (if_name == "switch_side_port") {
            return switchPort;
        } else {
            return SimObject::getPort(if_name, idx);
        }
    }

    AddrRangeList CXLSwitch::CPUSidePort::getAddrRanges() const {
        return owner->getAddrRanges();
    }

    void CXLSwitch::CPUSidePort::recvFunctional(PacketPtr pkt) {
        return owner->handleFunctional(pkt);
    }
                                                                                                                                                                          
    void CXLSwitch::handleFunctional(PacketPtr pkt) {
        memPort.sendFunctional(pkt);
        switchPort.sendFunctional(pkt);
    }

    AddrRangeList CXLSwitch::getAddrRanges() const {
        DPRINTF(CXLSwitch,"Sending new ranges\n");
        AddrRangeList listMem=memPort.getAddrRanges();
        listMem.merge(switchPort.getAddrRanges());
        return listMem; //merge two list 
    }

    void CXLSwitch::MemSidePort::recvRangeChange() {
        owner->sendRangeChange();
    }

    void CXLSwitch::sendRangeChange() {
        cpuPort.sendRangeChange();
    }

    bool CXLSwitch::CPUSidePort::recvTimingReq(PacketPtr pkt) {
        if(!owner->handleRequest(pkt)) {
            needRetry = true;
            return false;
        } else {
            return true;
        }
    }

    bool CXLSwitch::handleRequest(PacketPtr pkt) {
        if(blocked) {
            return false;
        }
        DPRINTF(CXLSwitch, "Got request for addr %#x\n", pkt->getAddr());
        blocked = true;
        memPort.sendPacket(pkt);
        switchPort.sendPacket(pkt);
        return true;
    }

    void CXLSwitch::MemSidePort::sendPacket(PacketPtr pkt) {
        panic_if(blockedPacket != nullptr,"should never try to send if blocked!");
        if(!sendTimingReq(pkt)) {
            blockedPacket = pkt;
        }
    }

    void CXLSwitch::MemSidePort::recvReqRetry() {
        assert(blockedPacket != nullptr);
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
    }

    bool CXLSwitch::MemSidePort::recvTimingResp(PacketPtr pkt) {
        return owner->handleResponse(pkt);
    }

    bool CXLSwitch::handleResponse(PacketPtr pkt) {
        assert(blocked);
        DPRINTF(CXLSwitch, "Got response for addr %#x\n", pkt->getAddr());
        blocked = false;
        cpuPort.sendPacket(pkt);

        cpuPort.trySendRetry();
        return true;
    }

    void CXLSwitch::CPUSidePort::sendPacket(PacketPtr pkt) {
        panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
        if(!sendTimingResp(pkt)) {
            blockedPacket = pkt;
        }
    }

    void CXLSwitch::CPUSidePort::recvRespRetry() {
        assert(blockedPacket != nullptr);
        PacketPtr pkt = blockedPacket;
        blockedPacket = nullptr;
        sendPacket(pkt);
    }

    void CXLSwitch::CPUSidePort::trySendRetry()
    {
        if (needRetry && blockedPacket == nullptr) {
            needRetry = false;
            DPRINTF(CXLSwitch, "Sending retry req for %d\n", id);
            sendRetryReq();
        }
    }

}