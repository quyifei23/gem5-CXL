//created by Quyifei
#ifndef __CXL_CXL_ENDPIONT__
#define __CXL_CXL_ENDPIONT__

#include "params/CXLEndpoint.hh"
#include "sim/sim_object.hh"
#include <tuple>
#include <string>
#include "base/types.hh"

namespace gem5{

    struct CXLLatency {
        Tick read;
        Tick write;
    };

    struct CXLBandwidth {
        double read;
        double write;
    };

    struct BandwidthPass {
        std::tuple<int, int> all_access;
        uint64_t read_config;
        uint64_t write_config;
    };

    struct LatencyPass {
        std::tuple<int, int> all_access;
        Tick dramlatency;
        Tick ma_ro;
        Tick ma_wb;
    };

    class CXLEndpoint : public SimObject{
        public:
            CXLLatency latency;
            CXLBandwidth bandwidth;

            virtual std::string output() = 0;
            virtual Tick calculate_latency(LatencyPass elem) = 0;
            virtual double calculate_bandwidth(BandwidthPass elem) = 0;
            virtual int insert(uint64_t timestamp, uint64_t phys_addr, uint64_t virt_addr, int index) = 0; //index 0:not this endpoint;1:store;2:load
            virtual std::tuple<int,int> get_all_access() = 0;
    };
}

#endif