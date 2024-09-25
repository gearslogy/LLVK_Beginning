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
    void setupDescriptors();
    void preparePipelines();
    void prepareUniformBuffers();
    void updateUniformBuffers();
    void bindResources();

    void recordCommandBuffer();

    void prepare() override {
        bindResources();
        loadTexture();
        loadModel();
        prepareUniformBuffers();
        setupDescriptors();
        preparePipelines();
    }
    void render() override {
        updateUniformBuffers();
        recordCommandBuffer();
    }


    // --- Geo & texture Resources ---
    struct  {
        GLTFLoader greenPlantGeo{}; // only use part0
        GLTFLoader yellowPlantGeo{};
        GLTFLoader largePlantGeo{};
        GLTFLoader groundGeo{};// only use part0
        VmaSimpleGeometryBufferManager geoBufferManager{};
    }Geos;
    struct {
        VmaUBOKTX2Texture greenPlant{};
        VmaUBOKTX2Texture yellowPlant{};
        VmaUBOKTX2Texture largePlant{};
        // simulate terrain
        VmaUBOKTX2Texture groundCliff{};
        VmaUBOKTX2Texture groundGrass{};
        VmaUBOKTX2Texture groundRock{};

    }Textures;
    VkSampler colorSampler{};
    // --- Geo & texture Resources ---



    struct {
        // binding = 0 : UBO
        // binding = 1 textures
        VkDescriptorSet greenPlant{};
        VkDescriptorSet yellowPlant{};
        VkDescriptorSet largePlant{};
        // --
        VkDescriptorSet terrainCliff{}; // simulate terrain
        VkDescriptorSet terrainGrass{};
        VkDescriptorSet terrainRock{};
    }descSets;
    VkDescriptorPool pool{};
    VkPipelineLayout pipelineLayout{};
    VkPipelineLayout terrainLayout{};
    // only one set  0:UBO 1:2d_array
    VkDescriptorSetLayout foliageDescSetLayout{};
    // only one set  0:UBO 1:2d_array_terrain grass, 2:2d_array_terrain yellow 3:2d_array_terrain rock
    VkDescriptorSetLayout opaqueDescSetLayout{};

    struct {
        VkPipeline foliage{};
        VkPipeline opaque{};
    }pipelines;

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
