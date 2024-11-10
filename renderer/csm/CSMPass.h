//
// Created by liuya on 11/8/2024.
//

#ifndef CSMPASS_H
#define CSMPASS_H


#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
#include "LLVK_VmaBuffer.h"
#include <glm/glm.hpp>
LLVK_NAMESPACE_BEGIN
class VulkanRenderer;
struct CSMPass {
    static constexpr uint32_t width = 2048;
    static constexpr uint32_t cascade_count = 4;
    void prepare();
    void cleanup();
    VulkanRenderer *pRenderer{};
    void recordCommandBuffer();

    struct {
        glm::mat4 lightViewProj[cascade_count];
    }uboData;
    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;


private:
    void prepareDepthResources();
    void prepareDepthRendePass();

    // depth resources
    VkRenderPass depthRenderPass{};
    VmaAttachment depthAttachment{}; // depthImage & framebuffer
    VkSampler depthSampler{};
    VkFramebuffer depthFramebuffer{};
    // pipeline
};

LLVK_NAMESPACE_END

#endif //CSMPASS_H
