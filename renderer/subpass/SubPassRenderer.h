//
// Created by liuyangping on 2025/2/26.
//
/*
 *
 * PHASE 1:
 * Generate depth from light
 *
 *
 *
 * PHASE 2:
 * swapchain_cd         RGBA8_unorm
 * Albedo               RGBA8_unorm
 * NRM                  RGBA16_SFLOAT
 * swapchain_depth      depth32
 * vBuffer              RGBA16_SFLOAT
 *
 *
 *
 */
#pragma once
#include <LLVK_UT_Pipeline.hpp>
#include "VulkanRenderer.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_VmaBuffer.h"
#include "SubpassTypes.hpp"
#include "SPShadowPass.h"
LLVK_NAMESPACE_BEGIN
struct SubPassResource;
struct SPShadowPass;
class SubPassRenderer : public VulkanRenderer {
public:
    SubPassRenderer();
    ~SubPassRenderer() override ;
    friend struct SPShadowPass;



protected:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;


    void createRenderpass() override; // automatically call in base::prepare()
    void createFramebuffers() override; // automatically call in base::prepare(); base::swapchainResize()
    void recordCommandBuffer();
    static inline constexpr glm::vec4 keyLightPos{7.7405f, 3.71551f, 2.52443f,1 };
private:
    subpass::GlobalUBO globalUBO{};
    struct Light{
        glm::vec4 position{};
        glm::vec3 color{};
        float radius{};
    };

    std::vector<Light> lights;


    void createGBufferAttachments();
    std::unique_ptr<SubPassResource> resourceLoader;
    std::unique_ptr<SPShadowPass> shadowPass;

    VkSampler colorSampler{};
    subpass::GBufferAttachments gBufferAttachments;


    struct {
        subpass::FramedSet gBufferBook{};
        subpass::FramedSet gBufferTelevision{};
        subpass::FramedSet gBufferWall{};
        subpass::FramedSet gBufferWoodenTable{};
        subpass::FramedSet comp{};
        subpass::FramedSet transparent{}; // for bottle
    }descSets;

    struct {
        VkDescriptorSetLayout gBuffer{};
        VkDescriptorSetLayout comp{};
        VkDescriptorSetLayout transparent{};
    }descSetLayouts;

    struct {
        VkPipelineLayout gBuffer{};
        VkPipelineLayout comp{};
        VkPipelineLayout transparent{};
    }pipelineLayouts;

    struct {
        VkPipeline gBuffer{};
        VkPipeline comp{};
        VkPipeline transparent{};
    }pipelines;

    subpass::FramedUBO uboBuffers{};
    subpass::FramedSSBO compSSBOBuffers{};


    void createDescPool();
    void prepareUBO();
    void prepareCompLightsSSBO();
    void updateCompLightsSSBO();
    void updatePreviousUBO();
    void updateCurrentUBO();
    void updateUBO();
    void prepareDescSets();
    void preparePipelines();
    VkDescriptorPool descPool{};


private:
    VkDevice usedDevice{};
    VkPhysicalDevice usedPhyDevice{};

    UT_GraphicsPipelinePSOs psoGBuffer{};
    UT_GraphicsPipelinePSOs psoComp{};
    UT_GraphicsPipelinePSOs psoTransparent{};

};


LLVK_NAMESPACE_END