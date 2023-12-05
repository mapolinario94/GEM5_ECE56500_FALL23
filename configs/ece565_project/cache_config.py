from m5.objects import Cache

class L1Cache(Cache):
    assoc = 8
    tag_latency = 5
    data_latency = 5
    response_latency = 5
    mshrs = 4
    tgts_per_mshr = 20
    def connectCPU(self, cpu):
        # need to define this in a base class!
        raise NotImplementedError
    def connectBus(self, bus):
        self.mem_side = bus.cpu_side_ports

class L1ICache(L1Cache):
    size = '32kB'
    def connectCPU(self, cpu):
        self.cpu_side = cpu.icache_port

class L1DCache(L1Cache):
    size = '32kB'
    def connectCPU(self, cpu):
        self.cpu_side = cpu.dcache_port

class L2Cache(Cache):
    size = '512kB'
    assoc = 8
    tag_latency = 12
    data_latency = 12
    response_latency = 12
    mshrs = 20
    tgts_per_mshr = 12
    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports
    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports

class L3Cache(Cache):
    size = '4MB'
    assoc = 16
    tag_latency = 39
    data_latency = 39
    response_latency = 39
    mshrs = 20
    tgts_per_mshr = 12
    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports
    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports