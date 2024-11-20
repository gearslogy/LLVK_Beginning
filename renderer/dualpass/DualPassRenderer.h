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
    DualPassRenderer();
    void cleanupObjects() override;
    void prepare() override;
    void render() override;

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

    void twoPassRender();
    void cmdRenderHair();
    void recordPass1();
    void recordPass2();

    struct {
        VmaAttachment colorAttachment;
        VmaAttachment depthAttachment;
    }renderTargets;
    struct {
        VkFramebuffer FBPass1;
        VkFramebuffer FBPass2;
    }frameBuffers;


    void createRenderTargets();
    void createFramebuffers();
    VkPipeline hairPipeline1{};
    VkPipeline hairPipeline2{};
    VkPipelineLayout dualPipelineLayout{};
    VkRenderPass renderpass1{};
    VkRenderPass renderpass2{};
};


LLVK_NAMESPACE_END
#endif //DUALPASSRENDERER_H
