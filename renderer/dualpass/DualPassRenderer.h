//
// Created by liuya on 11/11/2024.
//

#ifndef DUALPASSRENDERER_H
#define DUALPASSRENDERER_H


#include <LLVK_UT_Pipeline.hpp>

#include "VulkanRenderer.h"
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"
#include "renderer/public/GeometryContainers.h"
LLVK_NAMESPACE_BEGIN
struct DualPassRenderer : public VulkanRenderer{
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    struct {
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
    }uboData;

    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;
    VkDescriptorPool descPool{};
    VkDescriptorSetLayout descSetLayout{};
    std::array<VkDescriptorSet,2> descSets{};
private:
    // Resources
    GLTFLoader headLoader{};
    GLTFLoader hairLoader{};
    VmaUBOKTX2Texture tex{};
    VkSampler colorSampler{};
    VmaSimpleGeometryBufferManager geometryManager{};
    UT_GraphicsPipelinePSOs pso{};
};


LLVK_NAMESPACE_END
#endif //DUALPASSRENDERER_H
