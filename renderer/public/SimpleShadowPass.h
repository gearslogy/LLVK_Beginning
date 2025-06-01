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
    explicit SimpleShadowPass(const VulkanRenderer * renderer);
    virtual ~SimpleShadowPass();
    const VulkanRenderer *pRenderer;
    static constexpr auto size = 2048;

    // prepare order:
    /*
    // 2. render pass
    prepareRenderPass();
    // 3.framebuffer create
    prepareFramebuffer();
    // 4. UBO
    prepareUBO();
    // 5. dest sets
    prepareDescSetsAndPipelineLayout(); // DROPed use MainRenderer resources
    // 6. pipe
    preparePipeline();
     */
    virtual void drawObjects() = 0;
    void setLightPosition(glm::vec3 P) { keyLightPos = P;}
    void updateUBO();
    void cleanup();

    struct {
        VmaAttachment depthAttachment{};
        VkFramebuffer framebuffer{};
        VkRenderPass renderPass{};
        VkSampler depthSampler{};
    }shadowFramebuffer;

    // generated
    glm::mat4 depthMVP{};
    HLP::FramedUBO uboBuffers{}; // for depthMVP


    void prepareRenderPass();
    void prepareFramebuffer();
    void preparePipeline(const VkPipelineLayout &pl);
    void prepareUBO();
    void recordCommandBuffer();
private:
    // param to setting
    float near{0.1};
    float far{1000};
    glm::vec3 keyLightPos{};

    VkPipelineLayout pipelineLayout{};
    UT_GraphicsPipelinePSOs pipelinePSOs{};
    VkPipeline offscreenPipeline{};

};

LLVK_NAMESPACE_END

