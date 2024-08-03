#pragma once
#include "VulkanRenderer.h"

#define OBJECT_INSTANCES 300


#include "BufferManager.h"
#include "LLVK_Descriptor.hpp"
#include "LLVK_GeomtryLoader.h"

// Vertex layout for this example
// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
LLVK_NAMESPACE_BEGIN
struct DynamicsUBO : public VulkanRenderer{
    DynamicsUBO(): VulkanRenderer(){}

    // DATA:0 binding=0
    struct {
        glm::mat4 projection;
        glm::mat4 view;
    } uboVS;

    // DATA:1, 必须GPU对齐 binding=1;
    struct UboDataDynamic {
        glm::mat4* model{ nullptr };
    } uboDataDynamic;

    // BUFFER
    struct {
        UBOBuffer viewBuffer{};
        UBOBuffer dynamicBuffer{};
    } plantUniformBuffers;
    float scales[OBJECT_INSTANCES];
    glm::vec3 positions[OBJECT_INSTANCES];
    float yRotations[OBJECT_INSTANCES];

    struct {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    }uboStandardData;
    UBOBuffer standardUniformBuffer{};


    size_t dynamicAlignment{ 0 };
    static constexpr int plantNumPlantsImages = 6;
    VkPipelineLayout plantPipelineLayout{ VK_NULL_HANDLE };
    VkDescriptorPool descriptorPool{};
    VkDescriptorSetLayout plantUBOSetLayout{ VK_NULL_HANDLE };     // set = 0
    VkDescriptorSetLayout plantTextureSetLayout{ VK_NULL_HANDLE }; // set = 1
    VkDescriptorSet plantDescriptorSets[2]{ VK_NULL_HANDLE };
    VkPipeline plantPipeline{ VK_NULL_HANDLE };
    std::array<UBOTexture,6> plantTextures;

    VkSampler sampler{};


    struct {
        VkDescriptorSetLayout uboSetLayout{ VK_NULL_HANDLE };     // set = 0
        VkDescriptorSetLayout textureSetLayout{ VK_NULL_HANDLE }; // set = 1
        VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
        VkDescriptorSet sets[2]; // set = 0 for ubo, set=1 for texture
        VkPipeline pipeline;
    }standardPipeline; // gound pipeline we call this standard pipeline
    std::array<UBOTexture,5> groundTextures;


    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
    void setupDescriptors(
        );
    void preparePipelines();
    void prepareUniformBuffers();
    void updateUniformBuffers();
    void updateDynamicUniformBuffer();
    void bindResources();

    void recordCommandBuffer() override;

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
        updateDynamicUniformBuffer();
        recordCommandBuffer();
    }

    GLTFLoader plantGeo{};
    GLTFLoader groundGeo{};
    BufferManager geometryBufferManager{};

private:
    void loadPlantTextures();
    void loadGroundTextures();
};

LLVK_NAMESPACE_END
