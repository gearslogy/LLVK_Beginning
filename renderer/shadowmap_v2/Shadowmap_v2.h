//
// Created by liuya on 9/1/2024.
//
#pragma once
#include "VulkanRenderer.h"
#include "LLVK_VmaBuffer.h"
#include "LLVK_GeomtryLoader.h"
LLVK_NAMESPACE_BEGIN
struct Shadowmap_v2 : VulkanRenderer{
    Shadowmap_v2();
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
        float zNear{0.1};
        float zFar{1000.0};
    } uniformDataScene;

    struct {
        VkPipeline opacity;     // used for foliage render with depth map. forward rendering
        VkPipeline opaque;      // used for grid render with depth map     forward rendering
    }scenePipeline;
    VkDescriptorSetLayout sceneDescriptorSetLayout{}; // only one set=0 .
    VkPipelineLayout scenePipelineLayout{};       //only 1 set: binding=0 UBO, binding=1 colormaps_array, binding=2 shadowmap_v2


    // push constant data

    static constexpr float enable_opacity_texture{1};
    static constexpr float disable_opacity_texture{0};

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


    void recordCommandBuffer();

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



