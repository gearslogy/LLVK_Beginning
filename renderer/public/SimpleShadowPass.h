//
// Created by liuya on 4/25/2025.
//

#pragma once

#include <LLVK_SYS.hpp>
#include <LLVK_UT_Pipeline.hpp>
#include "Helper.hpp"
#include <glm/glm.hpp>
LLVK_NAMESPACE_BEGIN

class VulkanRenderer;
struct SimpleShadowPass {
    VulkanRenderer *pRenderer;
    VkDescriptorPool *pDescPool;

    static constexpr auto size = 2048;
    void prepare();
    void cleanup();
    virtual void drawObjects() = 0;
    void setLightPosition(glm::vec3 P) { keyLightPos = P;}
    void updateUBO();


    struct {
        VmaAttachment depthAttachment{};
        VkFramebuffer framebuffer{};
        VkRenderPass renderPass{};
        VkSampler depthSampler{};
    }shadowFramebuffer;

    // generated
    glm::mat4 depthMVP{};
    HLP::FramedUBO uboBuffers{};

private:
    void prepareRenderPass();
    void prepareFramebuffer();
    void preparePipeline();
    void prepareUBO();
    void prepareDescSets();

    void recordCommandBuffer();

    // param to setting
    float near{0.1};
    float far{1000};
    HLP::FramedSet sets{};
    glm::vec3 keyLightPos;


    UT_GraphicsPipelinePSOs pipelinePSOs{};
    VkPipeline offscreenPipeline{};
    VkDescriptorSetLayout offscreenDescriptorSetLayout{}; // only one set=0
    VkPipelineLayout offscreenPipelineLayout{};   //only 1 set: binding=0 UBO depthMVP, binding=1 colormap. use .a discard

};

LLVK_NAMESPACE_END

