//
// Created by liuya on 9/1/2024.
//
#pragma once
#include "VulkanRenderer.h"
#include "LLVK_VmaBuffer.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_UT_Pipeline.hpp"

LLVK_NAMESPACE_BEGIN
class ShadowMapPass;
class ScenePass;
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
    void updateUniformBuffers();

    glm::vec3 lightPos{};
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
    std::unique_ptr<ScenePass>     scenePass;

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



