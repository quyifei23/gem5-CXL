#created by Quyifei

from m5.SimObject import SimObject
from m5.params import *

class CXLEndpoint(SimObject):
    type = "CXLEndpoint"
    cxx_header = "cxl/cxl_endpoint.hh"
    cxx_class = "gem5::CXLEndpoint"

    
