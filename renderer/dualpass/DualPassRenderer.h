//
// Created by liuya on 11/11/2024.
//o

#ifndef DUALPASSRENDERER_H
#define DUALPASSRENDERER_H


#include <LLVK_UT_Pipeline.hpp>

#include "VulkanRenderer.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
struct OpaqueScenePass;
struct CompPass;
struct DualPassRenderer : public VulkanRenderer{
    DualPassRenderer();
    ~DualPassRenderer();
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    void swapChainResize() override;
    friend struct OpaqueScenePass;
    friend struct CompPass;
public:
    struct {
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
    }uboData;

    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;
    VkDescriptorPool descPool{};
    VkDescriptorSetLayout hairDescSetLayout{};
    std::array<VkDescriptorSet,2> hairDescSets{};
    void updateDualUBOs();
private:
    // Resources
    GLTFLoader headLoader{};
    GLTFLoader hairLoader{};
    GLTFLoader gridLoader{};
    VmaUBOKTX2Texture headTex{};
    VmaUBOKTX2Texture hairTex{};
    VmaUBOKTX2Texture gridTex{};
    VkSampler colorSampler{};
    VkSampler depthSampler{};
    VmaSimpleGeometryBufferManager geometryManager{};


    // render normal scene pipeline

    void recordAll();
    void recordHairPass1();
    void recordPass1DepthOnly();// for test. dropped
    void recordHairPass2();

    void cmdRenderHair();
    struct {
        VmaAttachment colorAttachment;
        VmaAttachment depthAttachment;
    }renderTargets;
    struct {
        VkFramebuffer FBPass1;
        VkFramebuffer FBPass2;
    }frameBuffersHairs;

    // system attachments COLOR + DEPTH
    void createRenderTargets();
    void cleanupRenderTargets();
    // hair fbs
    void createHairFramebuffers();
    void cleanupHairFramebuffers();

    VkPipeline hairPipeline1{};
    VkPipeline hairPipeline2{};
    VkPipelineLayout dualPipelineLayout{};
    VkRenderPass hairRenderPass1{};
    VkRenderPass hairRenderPass2{};

    // for rendering opaque
    std::unique_ptr<OpaqueScenePass> opaqueScenePass;
    // for comp
    std::unique_ptr<CompPass> compPass;

};


LLVK_NAMESPACE_END
#endif //DUALPASSRENDERER_H
