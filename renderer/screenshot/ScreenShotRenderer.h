//
// Created by liuya on 1/6/2025.
//

#pragma once

#include "LLVK_GeometryLoaderV2.hpp"
#include "renderer/public/CustomVertexFormat.hpp"
#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"
#include "LLVK_UT_Pipeline.hpp"
LLVK_NAMESPACE_BEGIN
class ScreenShotRenderer :public VulkanRenderer {
public:
    ScreenShotRenderer();
    void prepare() override;
    void cleanupObjects() override;
    void render() override;
    void recordCommandBuffer();
private:
    void capture();
    struct {
        VkImage srcImage;
        VkImage dstImage;
        VmaAllocation dstAllocation;
        bool supportSplit;
    }ssRes{};

    void prepareDescSets();
    void prepareUBO();
    void updateUBO();

    GLTFLoaderV2::Loader<VTXFmt_P_N> scene{};
    VmaSimpleGeometryBufferManager geomManager{};

    struct {// PVMIUBO ubo data
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
    }uboData{};

    using UBOFramedBuffers = std::array<VmaUBOBuffer, MAX_FRAMES_IN_FLIGHT>;
    using SetsFramed = std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>;
    UBOFramedBuffers uboBuffers{};
    VkDescriptorPool descPool{};
    VkDescriptorSetLayout descSetLayout{};
    SetsFramed descSets_set0{}; // for set=0 for ubo
    // scene pipeline
    void preparePipeline();
    VkPipeline scenePipeline{};
    VkPipelineLayout pipelineLayout{};
    UT_GraphicsPipelinePSOs pso;
};

LLVK_NAMESPACE_END


