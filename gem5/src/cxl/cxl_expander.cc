#include "cxl/cxl_expander.hh"
#include "debug/CXLExpander.hh"
#include "fmt/core.h"
namespace gem5 {
    CXLExpander::CXLExpander(int read_bw, int write_bw, int read_lat, int write_lat, int id, int capacity)
    : capacity(capacity),id(id) {
        this->bandwidth.read = read_bw;
        this->bandwidth.write = write_bw;
        this->latency.read = read_lat;
        this->latency.write = write_lat;
    }

    Tick CXLExpander::calculate_latency(LatencyPass elem) {
        auto all_access = elem.all_access;
        auto dramlatency = elem.dramlatency;
        auto ma_ro = elem.ma_ro;
        auto ma_wb = elem.ma_wb;
        auto all_read = std::get<0>(all_access);
        auto all_write = std::get<1>(all_access);
        double read_sample = 0.;
        if (all_read != 0) {
            read_sample = ((double)last_read / all_read);
        }
        double write_sample = 0.;
        if (all_write != 0) {
            write_sample = ((double)last_write / all_write);
        }
        this->last_latency =
            ma_ro * read_sample * (latency.read - dramlatency) + ma_wb * write_sample * (latency.write - dramlatency);
        return this->last_latency;
    }
    double CXLExpander::calculate_bandwidth(BandwidthPass elem) {
        // Iterate the map within the last 20ms
        auto all_access = elem.all_access;
        auto read_config = elem.read_config;
        auto write_config = elem.write_config;

        double res = 0.0;
        auto all_read = std::get<0>(all_access);
        auto all_write = std::get<1>(all_access);
        double read_sample = 0.;
        if (all_read != 0) {
            read_sample = ((double)last_read / all_read);
        }
        double write_sample = 0.;
        if (all_write != 0) {
            write_sample = ((double)last_write / all_write);
        }
        if ((((double)read_sample * 64 * read_config) / 1024 / 1024 / (this->epoch + this->last_latency) * 1000) >
            ((double)bandwidth.read)) {
            res +=
                read_sample * 64 * read_config / 1024 / 1024 / (this->epoch + this->last_latency) * 1000 / bandwidth.read -
                this->epoch * 0.001;
        }
        if ((((double)write_sample * 64 * write_config) / 1024 / 1024 / (this->epoch + this->last_latency) * 1000) >
            bandwidth.write) {
            res += (((double)write_sample * 64 * write_config) / 1024 / 1024 / (this->epoch + this->last_latency) * 1000 /
                    bandwidth.write) -
                this->epoch * 0.001;
        }
        return res;
    }

    int CXLExpander::insert(uint64_t timestamp, uint64_t phys_addr, uint64_t virt_addr, int index) {
        if (index == this->id) {
            last_timestamp = last_timestamp > timestamp ? last_timestamp : timestamp; // Update the last timestamp
            // Check if the address is already in the map)
            if (phys_addr != 0) {
                if (va_pa_map.find(virt_addr) == va_pa_map.end()) {
                    this->va_pa_map.emplace(virt_addr, phys_addr);
                } else {
                    this->va_pa_map[virt_addr] = phys_addr;
                    //LOG(INFO) << fmt::format("virt:{} phys:{} conflict insertion detected\n", virt_addr, phys_addr);
                }
                for (auto it = this->occupation.cbegin(); it != this->occupation.cend(); it++) {
                    if ((*it).second == phys_addr) {
                        this->occupation.erase(it);
                        this->occupation.emplace(timestamp, phys_addr);
                        this->load++;
                        return 2;
                    }
                }
                this->occupation.emplace(timestamp, phys_addr);
                this->store++;
                return 1;
            } else { // kernel mode access
                for (auto it = this->occupation.cbegin(); it != this->occupation.cend(); it++) {
                    if ((*it).second == virt_addr) {
                        this->occupation.erase(it);
                        this->occupation.emplace(timestamp, virt_addr);
                        this->load++;
                        return 2;
                    }
                }

                this->occupation.emplace(timestamp, virt_addr);
                this->store++;
                return 1;
            }

        } else {
            return 0;
        }
    }

    std::string CXLExpander::output() { return fmt::format("CXLExpander {}", this->id); }

    std::tuple<int, int> CXLExpander::get_all_access() {
    this->last_read = this->load - this->last_load;
    this->last_write = this->store - this->last_store;
    this->last_load = this->load;
    this->last_store = this->store;
    return std::make_tuple(this->last_read, this->last_write);
    }

}
