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
    VkDescriptorSetLayout hairDescSetLayout{};
    std::array<VkDescriptorSet,2> hairDescSets{};
    std::array<VkDescriptorSet,2> gridSets{};
    void updateDualUBOs();
private:
    // Resources
    GLTFLoader headLoader{};
    GLTFLoader hairLoader{};
    GLTFLoader gridLoader{};
    VmaUBOKTX2Texture hairTex{};
    VmaUBOKTX2Texture gridTex{};
    VkSampler colorSampler{};
    VkSampler depthSampler{};
    VmaSimpleGeometryBufferManager geometryManager{};


    // render normal scene pipeline
    struct {
        VkPipeline pipeline{};
        UT_GraphicsPipelinePSOs pso{};
    }sceneRendering;
    void prepareSceneRendering();
    void recordScene();

    void twoPassRender();
    void cmdRenderHair();
    void recordPass1();
    void recordPass1DepthOnly();
    void recordPass2();

    struct {
        VmaAttachment colorAttachment;
        VmaAttachment depthAttachment;
    }renderTargets;
    struct {
        VkFramebuffer FBPass1;
        VkFramebuffer FBPass2;
    }frameBuffersHairs;


    void createRenderTargets();
    void createHairFramebuffers();
    VkPipeline hairPipeline1{};
    VkPipeline hairPipeline2{};
    VkPipelineLayout dualPipelineLayout{};
    VkRenderPass hairRenderpass1{};
    VkRenderPass hairRenderpass2{};


    // comp resources
    void prepareComp();
    void cleanupComp();
    void recordComp();
    std::array<VkDescriptorSet,2> compSets;
    VkDescriptorSetLayout compSetLayout{};
    VkPipeline compPipeline{};
    VkPipelineLayout compPipelineLayout{};
    UT_GraphicsPipelinePSOs compPso{};

};


LLVK_NAMESPACE_END
#endif //DUALPASSRENDERER_H
