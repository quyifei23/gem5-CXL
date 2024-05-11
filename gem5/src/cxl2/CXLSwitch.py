from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import *

class CXLSwitch(ClockedObject):
    type = "CXLSwitch"
    cxx_header = "cxl2/cxl_switch.hh"
    cxx_class = "gem5::CXLSwitch"

    # each CXLSwtich has two CXLCtrl
    cpu_side_port = ResponsePort("CPU side port, receives requests")
    mem_side_port = RequestPort("MEM side port, connect to DRAM")
    switch_side_port = RequestPort("SWTICH side port, connect to switch")
    latency = Param.Cycles(1, "Cycles taken on switch")



