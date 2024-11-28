//
// Created by liuya on 11/8/2024.
//

#ifndef CSMPASS_H
#define CSMPASS_H


#include <LLVK_UT_Pipeline.hpp>
#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
#include "LLVK_VmaBuffer.h"
#include <glm/glm.hpp>
LLVK_NAMESPACE_BEGIN
class VulkanRenderer;

struct CSMPass {
    CSMPass(const VulkanRenderer *renderer, const VkDescriptorPool *descPool);

    static constexpr uint32_t width = 2048;
    static constexpr uint32_t cascade_count = 4;
    void prepare();
    void cleanup();

    void recordCommandBuffer();
    struct {
        glm::mat4 lightViewProj[cascade_count];
    }uboData;
    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;

private:
    void prepareDepthResources();
    void prepareDepthRenderPass();
    void prepareDescriptorSets();
    void preparePipelines();
    void prepareUniformBuffers();
    void updateCascade();
    // depth resources
    VkRenderPass depthRenderPass{};
    VmaAttachment depthAttachment{}; // depthImage & framebuffer
    VkSampler depthSampler{};
    VkFramebuffer depthFramebuffer{};
    // pipeline
    struct {
        VkDescriptorSetLayout setLayout{};
        UT_GraphicsPipelinePSOs pipelinePSOs{};
        VkPipelineLayout pipelineLayout{};
        VkPipeline pipeline{};
    }depthPOGeneric;

private:
    const VulkanRenderer *pRenderer{};
    const VkDescriptorPool *pDescriptorPool{};

};

LLVK_NAMESPACE_END

#endif //CSMPASS_H
