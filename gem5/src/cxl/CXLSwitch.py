from m5.params import *
from m5.objects.CXLEndpoint import CXLEndpoint

class CXLSwitch(CXLEndpoint):
    type = "CXLSwitch"
    cxx_header = "cxl/cxl_switch.hh"
    cxx_class = "gem5::CXLSwitch"

    bandwidth = Param.MemoryBandwidth("100MB/s")
    latency = Param.Latency(10)