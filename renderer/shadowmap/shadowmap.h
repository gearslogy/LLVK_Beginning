//
// Created by liuya on 9/1/2024.
//
#pragma once
#include "VulkanRenderer.h"
#include "LLVK_VmaBuffer.h"
#include "LLVK_GeomtryLoader.h"
LLVK_NAMESPACE_BEGIN
struct shadowmap : VulkanRenderer{
    void render() override;
    void cleanupObjects() override;
    void prepare() override;
    // user functions
    void loadTextures();
    void loadModels();
    void prepareUniformBuffers();
    void prepareDescriptorSets();
    void preparePipelines();
    void updateUniformBuffers();

    struct {
        glm::mat4 depthMVP;
    } uniformDataOffscreen;

    glm::vec3 lightPos{};
    struct UniformDataScene {
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model;
        glm::mat4 depthBiasMVP;
        glm::vec4 lightPos;
        // Used for depth map visualization
        float zNear{1.0};
        float zFar{96.0};
    } uniformDataScene;

    struct {
        VkPipeline opacity; // used for foliage depth map gen
        VkPipeline opaque;  // used for grid depth map gen
    }offscreenPipelines;
    VkDescriptorSetLayout offscreenDescriptorSetLayout{}; // only one set=0
    VkPipelineLayout offscreenPipelineLayout{};   //only 1 set: binding=0 UBO, binding=1 colormaps_array

    struct {
        VkPipeline opacity;     // used for foliage render with depth map. forward rendering
        VkPipeline opaque;      // used for grid render with depth map     forward rendering
    }scenePipeline;
    VkDescriptorSetLayout sceneDescriptorSetLayout{}; // only one set=0 .
    VkPipelineLayout scenePipelineLayout{};       //only 1 set: binding=0 UBO, binding=1 colormaps_array, binding=2 shadowmap


    // push constant data
    struct {
        float enable_opacity_texture{0};
    }constantData;

    struct {
        VmaUBOBuffer scene;     // final rendering : opaque and opacity use same UBO
        VmaUBOBuffer offscreen; // depth generate  : opaque and opacity use same UBO
    }uniformBuffers;


    struct {
        VkDescriptorSet opacity{};
        VkDescriptorSet opaque{};
    }offscreenSets;

    struct {
        VkDescriptorSet opacity{};
        VkDescriptorSet opaque{};
    }sceneSets;




    VkDescriptorPool descPool{};

    struct {
        uint32_t width{2048};
        uint32_t height{2048};
        VmaAttachment depthAttachment{};
        VkFramebuffer framebuffer{};
        VkRenderPass renderPass{};
        VkSampler depthSampler{};
    }shadowFramebuffer;
    void createOffscreenRenderPass();
    void createOffscreenFramebuffer();
    void cleanupOffscreenFramebuffer();


    VkSampler colorSampler{};
    VmaUBOKTX2Texture foliageTex;
    VmaUBOKTX2Texture gridTex;
    GLTFLoader gridGeo{};
    GLTFLoader foliageGeo{};
    VmaSimpleGeometryBufferManager geoBufferManager{};

private:
    inline void setRequiredObjects  (auto && ... ubo) {
        ((ubo.requiredObjects.device = this->mainDevice.logicalDevice),...);
        ((ubo.requiredObjects.physicalDevice = this->mainDevice.physicalDevice),...);
        ((ubo.requiredObjects.commandPool = this->graphicsCommandPool),...);
        ((ubo.requiredObjects.queue = this->mainDevice.graphicsQueue),...);
        ((ubo.requiredObjects.allocator = this->vmaAllocator),...);
    };

};


LLVK_NAMESPACE_END



