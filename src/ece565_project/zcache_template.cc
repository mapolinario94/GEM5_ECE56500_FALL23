#include "ece565_project/zcache_template.hh"

#include <iostream>

namespace gem5
{

ZCacheTemplate::ZCacheTemplate(const ZCacheTemplateParams &params) :
    SimObject(params)
{
    std::cout << "Hello World! From a SimObject!" << std::endl;
}

} // namespace gem5