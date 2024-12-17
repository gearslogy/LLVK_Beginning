//
// Created by liuya on 12/8/2024.
//

#ifndef RBDVATRENDERER_H
#define RBDVATRENDERER_H


#include <LLVK_UT_Pipeline.hpp>

#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"
#include "LLVK_ExrImage.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_GeometryLoaderV2.hpp"
LLVK_NAMESPACE_BEGIN
class RbdVatRenderer :public VulkanRenderer {
public:
    RbdVatRenderer();
    void prepare() override;
    void cleanupObjects() override;
    void render() override;
private:
    GLTFLoaderV2::Loader<GLTFVertexVATFracture> buildings{};
    VmaSimpleGeometryBufferManager geomManager{};
    // VAT
    VkSampler colorSampler{};
    VkSampler vatSampler{};
    VmaUBOKTX2Texture texDiff;
    VmaUBOExrRGBATexture texPosition;
    VmaUBOExrRGBATexture texOrient;


    // desc set
    struct {// PVMIUBO ubo data
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
        float frame;
    }uboData{};
    void prepareDescSets();
    void prepareUBO();
    void updateUBO();
    using UBOFramedBuffers = std::array<VmaUBOBuffer, MAX_FRAMES_IN_FLIGHT>;
    using SetsFramed = std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>;

    VkDescriptorSetLayout descSetLayout_set0{}; // for set=0
    VkDescriptorSetLayout descSetLayout_set1{}; // for set=1

    SetsFramed descSets_set0{}; // for set=0 for ubo
    SetsFramed descSets_set1{}; // for set=1 for textures

    UBOFramedBuffers uboBuffers{};
    VkDescriptorPool descPool{};
    // scene pipeline
    void preparePipeline();
    VkPipeline scenePipeline{};
    VkPipelineLayout pipelineLayout{};
    UT_GraphicsPipelinePSOs pso;
};
LLVK_NAMESPACE_END


#endif //RBDVATRENDERER_H
