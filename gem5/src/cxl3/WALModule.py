# //declare a machine with type transaction
# machine(MachineType:Transaction,"WAL transaction")
#     :Sequencer *sequencer;//Incoming request from CPU 
#     //Requests *to* the DRAM
#     MessageBuffer * requestToDram;
#     //Requests *to* the journal
#     MessageBuffer * requestToJournal;
#     //Responses *to* the cpuside
#     MessageBuffer * responseToCpu;
# {
#     state_declaration(State,desc="transaction states") {
#         RUNNING, AccessPermission:Available, desc="can receive new handles";

#         COMMITING, AccessPermission:NotAvailable, desc="can not receive new handles and committing to journal"

#         COMMITTED, AccessPermission:NotAvailable, desc="already committed to journal and can store to memory"

#         DESTROYED, AccessPermission:NotAvailable, desc="useless and need to be destroyed"
#     }
#     // Events that can be triggered on incoming messages.
#     // These are the events that will trigger transitions
#     enumeration(Event, desc="Transaction events") {
#         TimeOut, desc="transaction timer is up and forced to be committed";
#         BufferFull, desc="handle buffer is full";

#         JournalResp, desc="receive response from journal";
#     }


#     // A structure for the transaction. This stores the transaction data and state as define above 
#     structure(Entry, desc="handles queue") {
#         state TransactionState, desc="transaction state";
#         HandleBuffer Handles, desc="space to store handles";
#     }
    
# }

from m5.objects.SimObject import SimObject
from m5.params import *
from m5.proxy import *

class WALModule(SimObject):
    type = "WALModule"
    cxx_header = "cxl3/wal_module.hh"
    cxx_class = "gem5::WALModule"
    
    # transaction is connected to CPUside and JournalSide and MEMside
    cpu_side_port = ResponsePort("CPU side port, receives requests")
    mem_side_port = RequestPort("MEM side port, connect to mem")
