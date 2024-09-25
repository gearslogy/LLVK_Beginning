//
// Created by lp on 2024/9/25.
//

#ifndef SHADOWMAP_PASS_H
#define SHADOWMAP_PASS_H
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class VulkanRenderer;
class ShadowMapPass {
public:
    constexpr static uint32_t depth_width = 2048;
    constexpr static uint32_t depth_height = depth_width;
    void prepare(VulkanRenderer * pRenderer);
    void cleanup();
private:
    void createOffscreenDepthAttachment();
    void createOffscreenRenderPass();
    void createOffscreenFramebuffer();

    void prepareDescriptorSets();
    void preparePipelines();
private:
    VkPipeline offscreenPipeline{};
    VkDescriptorSetLayout offscreenDescriptorSetLayout{}; // only one set=0
    VkPipelineLayout offscreenPipelineLayout{};   //only 1 set: binding=0 UBO depthMVP, binding=1 colormap. use .a discard

    constexpr static void setRequiredObjects  (const auto *renderer, auto && ... ubo) {
        ((ubo.requiredObjects.device = renderer->mainDevice.logicalDevice),...);
        ((ubo.requiredObjects.physicalDevice = renderer->mainDevice.physicalDevice),...);
        ((ubo.requiredObjects.commandPool = renderer->graphicsCommandPool),...);
        ((ubo.requiredObjects.queue = renderer->mainDevice.graphicsQueue),...);
        ((ubo.requiredObjects.allocator = renderer->vmaAllocator),...);
    };

    struct {
        VmaAttachment depthAttachment{};
        VkFramebuffer framebuffer{};
        VkRenderPass renderPass{};
        VkSampler depthSampler{};
    }shadowFramebuffer;

    VulkanRenderer * renderer{};
};
LLVK_NAMESPACE_END


#endif //SHADOWMAP_PASS_H
