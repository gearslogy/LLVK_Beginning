//
// Created by liuyangping on 2024/8/7.
//
#pragma once
#include "GeoVertexDescriptions.h"
#include "VulkanRenderer.h"
#include "LLVK_VmaBuffer.h"
LLVK_NAMESPACE_BEGIN
struct ktx_texture : public VulkanRenderer {
    struct {
        glm::vec4 baseColor{1,0,0,0};
        glm::vec4 specColor{0,1,0,0};
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
        updateUniformBuffer();
        recordCommandBuffer();
    }
    struct {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorPool descPool;
        VkDescriptorSetLayout uboSetLayout; // set = 0
        VkDescriptorSetLayout texSetLayout; // set = 1
        VkDescriptorSet sets[2];
    }pipelineObject;


    Quad quad{};
    VmaSimpleGeometryBufferManager geoBufferManager{};
};

LLVK_NAMESPACE_END