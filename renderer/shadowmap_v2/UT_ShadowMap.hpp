//
// Created by lp on 2024/10/12.
//

#ifndef UT_SHADOWMAP_HPP
#define UT_SHADOWMAP_HPP


#include "LLVK_SYS.hpp"

LLVK_NAMESPACE_BEGIN

constexpr static void setRequiredObjects  (const auto *renderer, auto && ... ubo) {
    ((ubo.requiredObjects.device = renderer->getMainDevice().logicalDevice),...);
    ((ubo.requiredObjects.physicalDevice = renderer->getMainDevice().physicalDevice),...);
    ((ubo.requiredObjects.commandPool = renderer->getGraphicsCommandPool()),...);
    ((ubo.requiredObjects.queue = renderer->getGraphicsQueue()),...);
    ((ubo.requiredObjects.allocator = renderer->getVmaAllocator()),...);
};

LLVK_NAMESPACE_END




#endif //UT_SHADOWMAP_HPP
