//
// Created by liuya on 9/1/2024.
//
#pragma once
#include "VulkanRenderer.h"
#include "LLVK_VmaBuffer.h"
#include "LLVK_GeomtryLoader.h"
#include "LLVK_UT_Pipeline.hpp"

LLVK_NAMESPACE_BEGIN
class ShadowMapPass;

struct Shadowmap_v2 : VulkanRenderer{
    Shadowmap_v2();
    ~Shadowmap_v2() override;
    void render() override;
    void cleanupObjects() override;
    void prepare() override;
    // user functions
    void loadTextures();
    void loadModels();
    void createDescriptorPool();
    void prepareUniformBuffers();
    void prepareDescriptorSets();
    void preparePipelines();
    void updateUniformBuffers();


    glm::vec3 lightPos{};

    struct {
        VkPipeline opacity;     // used for foliage render with depth map. forward rendering
        VkPipeline opaque;      // used for grid render with depth map     forward rendering
    }scenePipeline{};
    VkDescriptorSetLayout sceneDescriptorSetLayout{}; // only one set=0 .
    VkPipelineLayout scenePipelineLayout{};       //only 1 set: binding=0 UBO, binding=1 colormaps_array, binding=2 shadowmap_v2

    struct {
        VmaUBOBuffer scene;     // final rendering : opaque and opacity use same UBO
    }uniformBuffers;
    struct {
        VkDescriptorSet opacity{};
        VkDescriptorSet opaque{};
    }sceneSets;
    UT_GraphicsPipelinePSOs scenePSO{};

    VkDescriptorPool descPool{};

    void recordCommandBuffer();

    // OBJECTS
    VkSampler colorSampler{};
    VmaUBOKTX2Texture foliageAlbedoTex{};  // RGB + A(to clip)
    VmaUBOKTX2Texture foliageOrdpTex{};    // R:trans G:rough B:Displace  A:white

    VmaUBOKTX2Texture gridAlbedoTex{};     // RGB + A(to clip)
    VmaUBOKTX2Texture gridOrdpTex{};       // R:AO G:rought B:Displace

    GLTFLoader gridGeo{};
    GLTFLoader foliageGeo{};
    VmaSimpleGeometryBufferManager geoBufferManager{};

    // SHADOW PASS
    std::unique_ptr<ShadowMapPass> shadowMapPass;


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



