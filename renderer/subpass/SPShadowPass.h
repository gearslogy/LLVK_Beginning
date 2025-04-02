//
// Created by Administrator on 3/2/2025.
//


#pragma once
#include <LLVK_SYS.hpp>
#include <LLVK_UT_Pipeline.hpp>
#include "SubpassTypes.hpp"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class SubPassRenderer;
struct SubPassResource;

struct SPShadowPass {
    SubPassRenderer *pRenderer{};  // renderer instanec
    SubPassResource *pResources{}; // All resources inlude geom tex pointer
    static constexpr auto size = 2048;
    void prepare();
    void cleanup();
    void recordCommandBuffer();
    void updateUBO(glm::vec3 lightPos);
private:
    void prepareRenderPass();
    void prepareFramebuffer();
    void preparePipeline();
    void prepareUBO();
    void prepareDescSets();


    // param to setting
    float near{0.1};
    float far{1000};
    // generated
    glm::mat4 depthMVP{};
    subpass::FramedUBO uboBuffers{};
    subpass::FramedSet sets{};


    struct {
        VmaAttachment depthAttachment{};
        VkFramebuffer framebuffer{};
        VkRenderPass renderPass{};
        VkSampler depthSampler{};
    }shadowFramebuffer;


    UT_GraphicsPipelinePSOs pipelinePSOs{};
    VkPipeline offscreenPipeline{};
    VkDescriptorSetLayout offscreenDescriptorSetLayout{}; // only one set=0
    VkPipelineLayout offscreenPipelineLayout{};   //only 1 set: binding=0 UBO depthMVP, binding=1 colormap. use .a discard

};

LLVK_NAMESPACE_END


