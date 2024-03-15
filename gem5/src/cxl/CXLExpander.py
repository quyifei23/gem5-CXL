from m5.params import *
from m5.objects.CXLEndpoint import CXLEndpoint


class CXLExpander(CXLEndpoint):
    type = "CXLExpander"
    cxx_header = "cxl/cxl_expander.hh"
    cxx_class = "gem5::CXLExpander"
    