//
// Created by liuya on 8/4/2024.
//

#ifndef TEX2DARRAY_H
#define TEX2DARRAY_H


#include "GeoVertexDescriptions.h"
#include "VulkanRenderer.h"
#include "LLVK_VmaBuffer.h"
#include "LLVK_VmaBuffer.h"
LLVK_NAMESPACE_BEGIN
struct ktx_tex2darray  : public VulkanRenderer{
    // Values passed to the shader per drawn instance
    static constexpr int MAX_LAYERS = 4;
    struct alignas(16) PerInstanceData {
        // Model matrix
        glm::mat4 model;
        // Layer index from which this instance will sample in the fragment shader
        float arrayIndex{ 0 };
    };
    struct {
        glm::mat4 proj;
        glm::mat4 view;
        PerInstanceData* instance{ nullptr };
    }uboData;

    VmaUBOBuffer uboBuffer{};
    VmaUBOKTX2Texture uboTexture{};
    VkSampler sampler{};
    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
    void setupDescriptors();
    void preparePipelines();
    void prepareUniformBuffers();
    void updateUniformBuffer();
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
        updateUniformBuffer();
        recordCommandBuffer();
        submitMainCommandBuffer();
        presentMainCommandBufferFrame();
    }
    struct {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorPool descPool;
        VkDescriptorSetLayout uboSetLayout; // set = 0
        VkDescriptorSetLayout texSetLayout; // set = 1
        VkDescriptorSet sets[2];
    }pipelineObject;


    ObjLoader geo{};
    VmaSimpleGeometryBufferManager geoBufferManager{};
};

LLVK_NAMESPACE_END

#endif //TEX2DARRAY_H
