#ifndef __CXL_CXL_EXPANDER_HH__
#define __CXL_CXL_EXPANDER_HH__

#include "cxl/cxl_endpoint.hh"
#include "cxl/cxl_switch.hh"
#include "params/CXLExpander.hh"
#include "base/types.hh"
#include <vector>
#include <string>
#include <map>

namespace gem5{
    class CXLExpander : public CXLEndpoint {
        public:
            CXLBandwidth bandwidth;
            CXLLatency latency;
            uint64_t capacity;
            std::map<uint64_t, uint64_t> occupation; // timestamp, pa
            std::map<uint64_t, uint64_t> va_pa_map; // va, pa
            int last_read = 0;
            int last_write = 0;
            Tick last_latency = 0.;
            uint64_t last_timestamp = 0;
            int id = -1;
            int load = 0;
            int store = 0;
            int last_load = 0;
            int last_store = 0;
            CXLExpander(int read_bw, int write_bw, int read_lat, int write_lat, int id, int capacity);
            std::tuple<int, int> get_all_access() override;
            int insert(uint64_t timestamp, uint64_t phys_addr, uint64_t virt_addr, int index) override;
            Tick calculate_latency(LatencyPass elem) override; // traverse the tree to calculate the latency
            double calculate_bandwidth(BandwidthPass elem) override;
            std::string output() override;
        };
}
#endif