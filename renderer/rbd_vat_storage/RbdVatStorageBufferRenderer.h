//
// Created by liuya on 12/24/2024.
//

#ifndef RBDVATSTORAGEBUFFERRENDERER_H
#define RBDVATSTORAGEBUFFERRENDERER_H

#include <LLVK_UT_Pipeline.hpp>
#include <utility>
#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"
#include "LLVK_ExrImage.h"
#include "LLVK_GeometryLoader.h"

#include "LLVK_GeometryLoaderV2.hpp"
#include "renderer/public/CustomVertexFormat.hpp"

LLVK_NAMESPACE_BEGIN

class RbdVatStorageBufferRenderer:public VulkanRenderer {
public:
    RbdVatStorageBufferRenderer();
protected:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;

    void recordCommandBuffer();
private:

    // desc set
    struct {// PVMIUBO ubo data
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
        glm::vec4 timeData; // use x for frame. y for numFrames
    }uboData{};


    int numFrames = 128;


    struct RBDData{
        glm::vec4 rbdP;
        glm::vec4 rbdOrient;
    };
    std::vector<RBDData> rbdData; // number of packs * number of frames
    VmaSSBOBuffer ssboBuffer;

    void updateTime();
    void prepareSSBO();
    void prepareUBO();
    void updateUBO();
    void prepareDescSets();
    void preparePipeline();
    void parseStorageData();



    GLTFLoaderV2::Loader<GLTFVertexVATFracture> buildings{};
    VmaSimpleGeometryBufferManager geomManager{};
    // VAT
    VkSampler colorSampler{};
    VmaUBOKTX2Texture texDiff;

    using UBOFramedBuffers = std::array<VmaUBOBuffer, MAX_FRAMES_IN_FLIGHT>;
    using SetsFramed = std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>;
    VkDescriptorSetLayout descSetLayout_set0{}; // for set=0
    VkDescriptorSetLayout descSetLayout_set1{}; // for set=1
    SetsFramed descSets_set0{}; // for set=0 for ubo
    SetsFramed descSets_set1{}; // for set=1 for textures
    UBOFramedBuffers uboBuffers{};
    VkDescriptorPool descPool{};




    VkPipeline scenePipeline{};
    VkPipelineLayout pipelineLayout{};
    UT_GraphicsPipelinePSOs pso;

    // time controller
    float tc_currentFrame = 0.0f;
    const float tc_frameRate = 24.0f;
    const float tc_frameTime = 1.0f / tc_frameRate;
    float tc_accumulator = 0.0f;
    uint32_t numPacks{0};
};

LLVK_NAMESPACE_END

#endif //RBDVATSTORAGEBUFFERRENDERER_H
