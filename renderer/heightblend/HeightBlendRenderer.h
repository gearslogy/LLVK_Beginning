//
// Created by liuyangping on 2025/1/6.
//

#ifndef HEIGHTBLENDRENDERER_H
#define HEIGHTBLENDRENDERER_H
#include <LLVK_UT_Pipeline.hpp>
#include <utility>
#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"
#include "LLVK_ExrImage.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_GeometryLoaderV2.hpp"
#include "renderer/public/CustomVertexFormat.hpp"

#include "VulkanRenderer.h"

LLVK_NAMESPACE_BEGIN
class HeightBlendRenderer : public VulkanRenderer {
public:
    HeightBlendRenderer();
    void prepare() override;
    void cleanupObjects() override;
    void render() override;
    void recordCommandBuffer() const;
private:
    GLTFLoaderV2::Loader<VTXFmt_P_N_T_UV0> grid{};
    VmaSimpleGeometryBufferManager geomManager{};
    VkSampler colorSampler{};
    VkSampler pointSampler{};
    VmaUBOKTX2Texture texDiff, texNDR;
    VmaUBOTexture texIndex;
    VmaUBOExrRGBATexture texExtUV;
    // desc set
    struct {// PVMIUBO ubo data
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
    }uboData{};
    void prepareDescSets();
    void prepareUBO();
    void updateUBO();
    using UBOFramedBuffers = std::array<VmaUBOBuffer, MAX_FRAMES_IN_FLIGHT>;
    using SetsFramed = std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>;

    VkDescriptorSetLayout descSetLayout_set0{}; // for set=0
    VkDescriptorSetLayout descSetLayout_set1{}; // for set=0
    SetsFramed descSets_set0{}; // for set=0 for ubo
    SetsFramed descSets_set1{}; // for set=0 for ubo

    UBOFramedBuffers uboBuffers{};
    VkDescriptorPool descPool{};
    // scene pipeline
    void preparePipeline();
    VkPipeline scenePipeline{};
    VkPipelineLayout pipelineLayout{};
    UT_GraphicsPipelinePSOs pso;

};
LLVK_NAMESPACE_END


#endif //HEIGHTBLENDRENDERER_H
