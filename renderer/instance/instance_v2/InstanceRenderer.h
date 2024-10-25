//
// Created by liuya on 8/16/2024.
//

#ifndef INSTANCE_H
#define INSTANCE_H


#include "VulkanRenderer.h"
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class InstancedObjectPass;
struct InstanceRendererV2 : public VulkanRenderer {
    InstanceRendererV2();
    ~InstanceRendererV2() override;
    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
    void createDescriptorPool();
    void updateUniformBuffers();
    void recordCommandBuffer();

    void prepare() override;
    void render() override {
        updateUniformBuffers();
        recordCommandBuffer();
        submitMainCommandBuffer();
        presentMainCommandBufferFrame();
    }

    // --- Geo & texture Resources ---
    glm::vec3 lightPos{-739.189,708.448,708.448};
    struct  {
        GLTFLoader terrain{};// only use part0
        VmaSimpleGeometryBufferManager geoBufferManager{};
    }geos;

    struct {
        // simulate terrain
        VmaUBOKTX2Texture albedoArray{};
        VmaUBOKTX2Texture normalArray{};
        VmaUBOKTX2Texture ordpArray{};
        VmaUBOKTX2Texture mask{};
    }terrainTextures;

    struct {
        std::vector<VmaUBOKTX2Texture> leaves{};
        std::vector<VmaUBOKTX2Texture> branch{};
        std::vector<VmaUBOKTX2Texture> root{};


    }treesTextures;



    VkSampler colorSampler{};
    // --- Geo & texture Resources ---
    VkDescriptorPool descPool{};
    std::unique_ptr<InstancedObjectPass> instancePass;

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


#endif //INSTANCE_H
