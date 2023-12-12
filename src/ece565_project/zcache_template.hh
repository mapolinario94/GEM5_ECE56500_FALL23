#ifndef __ECE565_PROJECT_ZCACHE_TEMPLATE_HH__
#define __ECE565_PROJECT_ZCACHE_TEMPLATE_HH__

#include "params/ZCacheTemplate.hh"
#include "sim/sim_object.hh"

namespace gem5
{

class ZCacheTemplate : public SimObject
{
  public:
    ZCacheTemplate(const ZCacheTemplateParams &p);
};

} // namespace gem5

#endif // __LEARNING_GEM5_HELLO_OBJECT_HH__