from m5.params import *
from m5.SimObject import SimObject

class ZCacheTemplate(SimObject):
    type = 'ZCacheTemplate'
    cxx_header = "ece565_project/zcache_template.hh"
    cxx_class = "gem5::ZCacheTemplate"