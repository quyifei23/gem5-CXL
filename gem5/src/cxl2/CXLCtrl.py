#created by Quyifei

from m5.objects.MemCtrl import *
from m5.objects.MemInterface import MemInterface
from m5.params import *
from m5.proxy import *

# here define SwitchInterface
class SwitchInterface(MemInterface):
    type = "SwitchInterface"
    cxx_headr = "cxl2/switch_interface.hh"
    cxx_class = "gem5::SwitchInterface"

    
# CXLCtrl is based on MemCtrl. It can connectd to DRAM or a Switch

class CXLCtrl(MemCtrl):
    type = "CXLCtrl"
    cxx_headr = "cxl2/cxl_ctrl.hh"
    cxx_class = "gem5::CXLCtrl"

    # MemCtrl already has a response port, and a dram to get the mem interface
    # So we should add a Switch port

    SwitchPort = ResponsePort("This port responds to Switch requests")
    
    # interface to Switch
    switch_ = Param.SwitchInterface("Switch interface")

