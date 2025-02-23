//
// Created by liuyangping on 2025/2/8.
//

#ifndef TANK_BILLBOARD_H
#define TANK_BILLBOARD_H

#include <LLVK_UT_Pipeline.hpp>

#include "VulkanRenderer.h"
#include "LLVK_GeometryLoaderV2.hpp"
#include "renderer/public/CustomVertexFormat.hpp"
#include "renderer/public/UT_CustomRenderer.hpp"


LLVK_NAMESPACE_BEGIN
struct tank_billboard : VulkanRenderer{
    struct InstanceData {
        glm::vec3 P;
        glm::vec4 orient;
        float pscale;
    };
protected:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
private:
    void createInstanceBuffer();
    void recordCommandBuffer() const;
    void createDescPool();
    void createDescSetsAndDescSetLayout();
    void updateUBO();
private:
    GLTFLoaderV2::Loader<VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID> mGeoLoader;
    UT_GraphicsPipelinePSOs pso;
    VmaSimpleGeometryBufferManager geomManager{};
    struct {
        glm::mat4 proj{};
        glm::mat4 view{};
        glm::mat4 model{};
        glm::vec4 camPos{};
    }uboData;

    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descSets{};
    VkSampler colorSampler{};
    VmaUBOTexture diffTex{};
    VmaUBOTexture transTex{};
    VkDescriptorSetLayout setLayout{}; // for set=0
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
    VkDescriptorPool descPool{};

    VkBuffer instanceBuffer{}; // do not clear, auto clear by geomManager
    size_t instanceCount{};
};
LLVK_NAMESPACE_END


#endif //TANK_BILLBOARD_H
