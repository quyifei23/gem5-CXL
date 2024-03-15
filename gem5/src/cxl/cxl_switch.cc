#include "cxl/cxl_switch.hh"
#include "debug/CXLSwitch.hh"
#include "fmt/core.h"
namespace gem5
{
    CXLSwitch::CXLSwitch(int id) {this->id = id;}

    Tick CXLSwitch::calculate_latency(LatencyPass elem) {
        Tick lat = 0;
        for(auto &expander : this->expanders) {
            lat += expander->calculate_latency(elem);
        }
        for(auto &switch_ : this->switches) {
            lat += switch_->calculate_latency(elem);
        }
        return lat;
    }

    double CXLSwitch::calculate_bandwidth(BandwidthPass elem) {
        double bw = 0.0;
        for(auto &expander : this->expanders) {
            bw += expander->calculate_bandwidth(elem);
        }
        for(auto &switch_ : this->switches) {
            bw += switch_->calculate_bandwidth(elem);
        }
        return bw;
    }

    int CXLSwitch::insert(uint64_t timestamp,uint64_t phys_addr, uint64_t virt_addr, int index) {
        for(auto &expander : this->expanders) {
            int ret = expander->insert(timestamp,phys_addr, virt_addr, index);
            if( ret == 1 ) {
                this->store++;
                return 1;
            } else if(ret == 2) {
                this->load++;
            } else {
                return 0;
            }
        }

        for(auto &switch_ : this->switches) {
            int ret = switch_->insert(timestamp,phys_addr, virt_addr, index);
            if( ret == 1 ) {
                this->store++;
                return 1;
            } else if(ret == 2) {
                this->load++;
            } else {
                return 0;
            }
        }
    }

    std::string CXLSwitch::output() {
        std::string res = fmt::format("CXLSwitch {} ", this->id);
        if (!this->switches.empty()) {
            res += "(";
            res += this->switches[0]->output();
            for (size_t i = 1; i < this->switches.size(); ++i) {
                res += ",";
                res += this->switches[i]->output();
            }
            res += ")";
        } else if (!this->expanders.empty()) {
            res += "(";
            res += this->expanders[0]->output();
            for (size_t i = 1; i < this->expanders.size(); ++i) {
                res += ",";
                res += this->expanders[i]->output();
            }
            res += ")";
        }
        return res;
    }

    std::tuple<int,int> CXLSwitch::get_all_access() {
        int read = 0, write = 0;
        for (auto &expander : this->expanders) {
            auto [r, w] = expander->get_all_access();
            read += r;
            write += w;
        }
        for (auto &switch_ : this->switches) {
            auto [r, w] = switch_->get_all_access();
            read += r;
            write += w;
        }
        return std::make_tuple(read, write);
    }
}