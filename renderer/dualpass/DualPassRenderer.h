//
// Created by liuya on 11/11/2024.
//

#ifndef DUALPASSRENDERER_H
#define DUALPASSRENDERER_H


#include <LLVK_UT_Pipeline.hpp>

#include "VulkanRenderer.h"
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
struct DualPassRenderer : public VulkanRenderer{
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    void swapChainResize() override;
    struct {
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
    }uboData;

    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;
    VkDescriptorPool descPool{};
    VkDescriptorSetLayout descSetLayout{};
    std::array<VkDescriptorSet,2> dualDescSets{};
    void updateDualUBOs();
private:
    // Resources
    GLTFLoader headLoader{};
    GLTFLoader hairLoader{};
    VmaUBOKTX2Texture tex{};
    VkSampler colorSampler{};
    VkSampler depthSampler{};
    VmaSimpleGeometryBufferManager geometryManager{};
    VkFramebuffer dualFrameBuffer{};
    VmaAttachment dualColorAttachment{};
    VmaAttachment dualDepthAttachment{};

    UT_GraphicsPipelinePSOs dualPso{};
    VkPipeline frontPipeline{};
    VkPipeline backPipeline{};
    VkPipelineLayout dualPipelineLayout{};  // 需要提前创建


    VkCommandBuffer mrtCommandBuffers[MAX_FRAMES_IN_FLIGHT]{};
    VkSemaphore mrtSemaphores[MAX_FRAMES_IN_FLIGHT]{};
    void createDualCommandBuffers();
    void createDualPipelines();
    void createDualRenderPass();
    void createDualAttachment();
    void createDualFrameBuffer();
    void cleanupDualAttachment();
    VkRenderPass dualRenderPass{};
    void recordCommandDual();

// FOR Composition
private:
    void prepareComp();
    void cleanupComp();
    void recordCommandComp();
    VkDescriptorSetLayout compDescSetLayout{};// one set. bind dualColorAttachment
    VkPipelineLayout compPipelineLayout{};
    VkPipeline compPipeline{};
    std::array<VkDescriptorSet,2> compDescSets{};
    UT_GraphicsPipelinePSOs compPso{};

};


LLVK_NAMESPACE_END
#endif //DUALPASSRENDERER_H
