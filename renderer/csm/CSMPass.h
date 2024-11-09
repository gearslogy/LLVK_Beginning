//
// Created by liuya on 11/8/2024.
//

#ifndef CSMPASS_H
#define CSMPASS_H


#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
#include "LLVK_VmaBuffer.h"
LLVK_NAMESPACE_BEGIN
class VulkanRenderer;
struct CSMPass {
    static constexpr uint32_t width = 2048;
    void prepare();
    void cleanup();
    void prepareDepthResources();

    VulkanRenderer *pRenderer;
private:
    VkRenderPass depthPass{};
    VmaAttachment depthAttachment{};
    VkSampler depthSampler{};
};

LLVK_NAMESPACE_END

#endif //CSMPASS_H
