# import the m5 (gem5) library created when gem5 is built
import m5
import caches
# import all of the SimObjects
from m5.objects import *

# create the system we are going to simulate
system = System()

# Set the clock frequency of the system (and all of its children)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "2GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = "timing"  # Use timing accesses
system.mem_ranges = [AddrRange("512MB"),AddrRange("512MB","1024MB"),AddrRange("1024MB","1536MB"),AddrRange("1536MB","2048MB")]  # Create an address range

# Create a simple CPU
system.cpu = X86TimingSimpleCPU()

# Create the simple memory object
system.memobj = SimpleMemobj()

# Hook the CPU ports up to the cache
system.cpu.icache_port = system.memobj.inst_port
system.cpu.dcache_port = system.memobj.data_port

# Create a memory bus, a coherent crossbar, in this case
system.membus = SystemXBar()

# Connect the memobj
system.memobj.mem_side = system.membus.cpu_side_ports

# create the interrupt controller for the CPU and connect to the membus
system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports


# create the CXLSwitch
system.cxlswitch = CXLSwitch()
system.cxlswitch.cpu_side_port = system.membus.mem_side_ports

# Create a DDR3 memory controller and connect it to the membus
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.cxlswitch.mem_side_port


# create another CXLSwitch
system.cxlswitch2 = CXLSwitch()
system.cxlswitch2.cpu_side_port = system.cxlswitch.switch_side_port


system.mem_ctrl2 = MemCtrl()
system.mem_ctrl2.dram = DDR3_1600_8x8()
system.mem_ctrl2.dram.range = system.mem_ranges[2]
system.mem_ctrl2.port = system.cxlswitch2.mem_side_port

system.mem_ctrl3 = MemCtrl()
system.mem_ctrl3.dram = DDR3_1600_8x8()
system.mem_ctrl3.dram.range = system.mem_ranges[3]
system.mem_ctrl3.port = system.cxlswitch2.switch_side_port


# Connect the system up to the membus
system.system_port = system.membus.cpu_side_ports




# Create a process for a simple "Hello World" application
process = Process()
# Set the command
# grab the specific path to the binary
thispath = os.path.dirname(os.path.realpath(__file__))
binpath = os.path.join(
    thispath, "../../",
    "tests/test-progs/hello/bin/x86/linux/hello",
)
# cmd is a list which begins with the executable (like argv)
process.cmd = [binpath]
# Set the cpu to use the process as its workload and create thread contexts
system.cpu.workload = process
system.cpu.createThreads()

system.workload = SEWorkload.init_compatible(binpath)

# set up the root SimObject and start the simulation
root = Root(full_system=False, system=system)
# instantiate all of the objects we've created above
m5.instantiate()

print(f"Beginning simulation!")
exit_event = m5.simulate()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
