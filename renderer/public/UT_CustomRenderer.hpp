//
// Created by lp on 2024/10/12.
//

#ifndef UT_SHADOWMAP_HPP
#define UT_SHADOWMAP_HPP


#include "LLVK_SYS.hpp"

LLVK_NAMESPACE_BEGIN

constexpr void setRequiredObjectsByRenderer  (const auto *renderer, Concept::has_requiredObjects auto && ... ubo) {
    ((ubo.requiredObjects.device = renderer->getMainDevice().logicalDevice),...);
    ((ubo.requiredObjects.physicalDevice = renderer->getMainDevice().physicalDevice),...);
    ((ubo.requiredObjects.commandPool = renderer->getGraphicsCommandPool()),...);
    ((ubo.requiredObjects.queue = renderer->getGraphicsQueue()),...);
    ((ubo.requiredObjects.allocator = renderer->getVmaAllocator()),...);
};

constexpr void setRequiredObjectsByRenderer  (const auto *renderer, Concept::is_range auto && range_ubo) {
    for(auto &o : range_ubo) {
        o.requiredObjects.device = renderer->getMainDevice().logicalDevice;
        o.requiredObjects.physicalDevice = renderer->getMainDevice().physicalDevice;
        o.requiredObjects.commandPool = renderer->getGraphicsCommandPool();
        o.requiredObjects.queue = renderer->getGraphicsQueue();
        o.requiredObjects.allocator = renderer->getVmaAllocator();
    }
};


namespace UT_STR {
    inline std::string replace(const std::string& str, const std::string& from, const std::string& to) {
        std::string result = str;
        if (auto pos = result.find(from); pos != std::string::npos) {
            result.replace(pos, from.length(), to);
        }
        return result;
    }
}



LLVK_NAMESPACE_END




#endif //UT_SHADOWMAP_HPP
