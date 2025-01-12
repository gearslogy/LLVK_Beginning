//
// Created by lp on 2024/10/12.
//

#ifndef UT_SHADOWMAP_HPP
#define UT_SHADOWMAP_HPP


#include "LLVK_SYS.hpp"

LLVK_NAMESPACE_BEGIN


struct GetRendererPtr {
    static auto * value(auto object) {
        using T =std::remove_pointer_t< std::decay_t<decltype(object)>  >;

        if constexpr(Concept::has_pRenderer<T>)
            return object->pRenderer;
        else
            return object;
    }
};

constexpr void setRequiredObjectsByRenderer  (const auto *renderer_or_object, Concept::has_requiredObjects auto && ... ubo) {
    const auto *renderer = GetRendererPtr::value(renderer_or_object);
    ((ubo.requiredObjects.device = renderer->getMainDevice().logicalDevice),...);
    ((ubo.requiredObjects.physicalDevice = renderer->getMainDevice().physicalDevice),...);
    ((ubo.requiredObjects.commandPool = renderer->getGraphicsCommandPool()),...);
    ((ubo.requiredObjects.queue = renderer->getGraphicsQueue()),...);
    ((ubo.requiredObjects.allocator = renderer->getVmaAllocator()),...);
};

constexpr void setRequiredObjectsByRenderer  (const auto *renderer_or_object, Concept::is_requiredObjects auto && ... ubo) {
    const auto *renderer = GetRendererPtr::value(renderer_or_object);
    ((ubo.device = renderer->getMainDevice().logicalDevice),...);
    ((ubo.physicalDevice = renderer->getMainDevice().physicalDevice),...);
    ((ubo.commandPool = renderer->getGraphicsCommandPool()),...);
    ((ubo.queue = renderer->getGraphicsQueue()),...);
    ((ubo.allocator = renderer->getVmaAllocator()),...);
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
