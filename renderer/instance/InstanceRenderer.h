//
// Created by liuya on 8/16/2024.
//

#ifndef INSTANCE_H
#define INSTANCE_H


#include "VulkanRenderer.h"
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
struct InstanceRenderer : public VulkanRenderer {

    InstanceRenderer(): VulkanRenderer(){}

    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
    void createDescriptorPool();
    void preparePipelines();
    void prepareUniformBuffers();
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
    struct  {
        GLTFLoader terrain{};// only use part0
        VmaSimpleGeometryBufferManager geoBufferManager{};
    }geos;

    struct {
        // simulate terrain
        VmaUBOKTX2Texture groundCliff{};
        VmaUBOKTX2Texture groundGrass{};
        VmaUBOKTX2Texture groundRock{};
    }Textures;
    VkSampler colorSampler{};
    // --- Geo & texture Resources ---
    VkDescriptorPool descPool{};

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
