//
// Created by liuya on 11/8/2024.
//

#ifndef CSMPASS_H
#define CSMPASS_H


#include <LLVK_UT_Pipeline.hpp>
#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
#include "LLVK_VmaBuffer.h"
#include <glm/glm.hpp>

LLVK_NAMESPACE_BEGIN
class CSMRenderer;

struct CSMDepthPass {
    explicit CSMDepthPass(const CSMRenderer *renderer);

    static constexpr uint32_t width = 2048;
    static constexpr uint32_t cascade_count = 4;
    void prepare();
    void cleanup();

    void recordCommandBuffer();
    struct {
        glm::mat4 lightViewProj[cascade_count];
    }uboGeomData;
    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboGeomBuffer;// FRAMED BUFFER

    // FS binding. Before we do this we have to perform a depthPass prepare!
    struct {
        float cascadeSplits[cascade_count];
        glm::vec4 lightDir;
    }fsUBOData;
     std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboFSBuffer;

    [[nodiscard]] const auto& renderTarget() const {return depthAttachment;}
    void update(); // update cascade and uboData
private:
    void prepareDepthResources();
    void prepareDepthRenderPass();

    void preparePipelines();
    void prepareUniformBuffers();

    // depth resources
    VkRenderPass depthRenderPass{};
    VmaAttachment depthAttachment{}; // depthImage & framebuffer
    VkSampler depthSampler{};
    VkFramebuffer depthFramebuffer{};

    // pipeline
    UT_GraphicsPipelinePSOs pso{};
    VkPipeline instancePipeline{};
    VkPipeline normalPipeline{};


private:
    const CSMRenderer *pRenderer{};
    const VkDescriptorPool *pDescriptorPool{};
    float cascadeSplitLambda = 0.95f;
    std::array<float, cascade_count> cascadesSplitDepth{};
    std::array<glm::mat4, cascade_count> cascadesViewProjMatrix{};
};

LLVK_NAMESPACE_END

#endif //CSMPASS_H
