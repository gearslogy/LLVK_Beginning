//
// Created by star on 4/24/2024.
//

#ifndef RENDERPASS_H
#define RENDERPASS_H
#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"


LLVK_NAMESPACE_BEGIN
struct RenderPass {
    VkDevice bindDevice{}; // bind device
    VkPhysicalDevice bindPhysicalDevice{};
    // created object
    VkRenderPass pass{};
    // function
    void init();
    void cleanup();
};
LLVK_NAMESPACE_END


#endif //RENDERPASS_H
