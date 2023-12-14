import argparse
import m5
from m5.objects import *
from m5.defines import buildEnv
from m5.util import addToPath, fatal, warn
from cache_config import *

addToPath('../')

from spec import spec2k6_spec2k17

parser = argparse.ArgumentParser(
    description='A simple system with 3-level cache.')
parser.add_argument(
    "--benchmark", default="", nargs="?", type=str, help="Benchmark.")
parser.add_argument(
    "--indexing-policy", default="set", type=str, choices=["skew", "set"])
parser.add_argument(
    "--max-instructions", default=50e7, type=int)
options = parser.parse_args()

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('8GB')]

system.cpu = X86O3CPU()



# system.cpu.icache_port = system.membus.cpu_side_ports
# system.cpu.dcache_port = system.membus.cpu_side_ports

system.cpu.icache = L1ICache()
system.cpu.dcache = L1DCache()

system.cpu.icache.connectCPU(system.cpu)
system.cpu.dcache.connectCPU(system.cpu)

system.l2bus = L2XBar()
system.cpu.icache.connectBus(system.l2bus)
system.cpu.dcache.connectBus(system.l2bus)


system.l2cache = L2Cache(tags=ZBaseSetAssoc(indexing_policy=ZSkewed()), replacement_policy=ZLRURP(), zcache_bool=True)
system.l2cache.connectCPUSideBus(system.l2bus)
system.membus = SystemXBar()
system.l2cache.connectMemSideBus(system.membus)

system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

system.system_port = system.membus.cpu_side_ports

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR4_2400_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

process = spec2k6_spec2k17.get_process(
    options, buildEnv['TARGET_ISA'])

if buildEnv['TARGET_ISA'] == 'x86':
    system.kvm_vm = KvmVM()
    system.m5ops_base = 0xffff0000
    # for process in multiprocesses:
    process.useArchPT = True
    process.kvmInSE = True
else:
    fatal("KvmCPU can only be used in SE mode with x86")

# binary = 'tests/test-progs/hello/bin/x86/linux/hello'
# for gem5 V21 and beyond
system.workload = SEWorkload.init_compatible(process.executable)

# process = Process()
# process.cmd = [binary]
system.cpu.workload = process
system.cpu.createThreads()

# max number of instruction for simulation
system.cpu.max_insts_any_thread = options.max_instructions

root = Root(full_system = False, system = system)
m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate()

print('Exiting @ tick {} because {}'.format(
    m5.curTick(), exit_event.getCause()))