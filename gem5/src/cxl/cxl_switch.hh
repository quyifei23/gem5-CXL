
#ifndef __CXL_CXL_SWITCH_HH__
#define __CXL_CXL_SWITCH_HH__

#include "cxl/cxl_endpoint.hh"
#include "cxl/cxl_expander.hh"
#include "params/CXLSwitch.hh"
#include "base/types.hh"
#include <vector>
#include <string>
namespace gem5
{
    class CXLSwitch : public CXLEndpoint{
        public:
            uint64_t load = 0;
            uint64_t store = 0;
            int id = -1;
            std::vector<CXLSwitch *> switches{};
            std::vector<CXLExpander *> expanders{};
            CXLSwitch(int id);
            std::tuple<int,int> get_all_access() override;
            Tick calculate_latency(LatencyPass elem) override; // traverse the tree to calculate the latency
            double calculate_bandwidth(BandwidthPass elem) override;
            int insert(uint64_t timestamp, uint64_t phys_addr, uint64_t virt_addr, int index) override;
            std::string output() override;
    };
}
#endif