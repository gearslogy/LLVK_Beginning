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
struct SetRequiredObjectsIMPL {
    constexpr static void set(const auto* renderer, auto& obj) {
        const auto& device = renderer->getMainDevice();
        obj.device = device.logicalDevice;
        obj.physicalDevice = device.physicalDevice;
        obj.commandPool = renderer->getGraphicsCommandPool();
        obj.queue = renderer->getGraphicsQueue();
        obj.allocator = renderer->getVmaAllocator();
    }
};

constexpr void setRequiredObjectsByRenderer  (const auto *renderer_or_object, Concept::has_requiredObjects auto && ... req) {
    const auto *renderer = GetRendererPtr::value(renderer_or_object);
    (SetRequiredObjectsIMPL::set(renderer, req.requiredObjects), ...);
};

constexpr void setRequiredObjectsByRenderer  (const auto *renderer_or_object, Concept::is_requiredObjects auto && ... req) {
    const auto *renderer = GetRendererPtr::value(renderer_or_object);
    (SetRequiredObjectsIMPL::set(renderer, req), ...);
};

constexpr void setRequiredObjectsByRenderer  (const auto *renderer_or_object, Concept::is_range auto && range_ubo) {
    const auto *renderer = GetRendererPtr::value(renderer_or_object);
    for(auto &&o : range_ubo) SetRequiredObjectsIMPL::set(renderer, o.requiredObjects);
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
