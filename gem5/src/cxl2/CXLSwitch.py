from m5.objects.CXLCtrl import CXLCtrl
from m5.params import *
from m5.proxy import *

class CXLSwitch(CXLCtrl):
    type = "CXLSwitch"
    cxx_headr = "cxl2/cxl_switch.hh"
    cxx_class = "gem5::CXLSwitch"

    # each CXLSwtich has two CXLCtrl
    CXLPort = Param.

