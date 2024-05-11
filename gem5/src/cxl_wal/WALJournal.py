from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import *

class WALJournal(ClockedObject):
    type = "WALJournal"
    cxx_header = "cxl_wal/wal_journal.hh"
    cxx_class = "gem5::WALJournal"

#this module select the write request while passing read request
#then store the write request 
    cpu_side_port = ResponsePort("CPU side port, receives requests")
    mem_side_port = RequestPort("MEM side port, connect to DRAM")
    latency = Param.Cycles(1, "Cycles taken on store")
