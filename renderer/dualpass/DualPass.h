//
// Created by liuya on 11/11/2024.
//

#ifndef DUALPASS_H
#define DUALPASS_H

#include <LLVK_UT_Pipeline.hpp>
#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN

struct DualPassRenderer;
// our first render pass
struct OpaqueScenePass {
    OpaqueScenePass(DualPassRenderer *renderer);
    void prepare();
    void recordCommandBuffer();
    void cleanup();
    void onSwapChainResize();
    // bind object
private:
    // created
    void createRenderPass();
    void createFramebuffer();
    void createPipeline();
    DualPassRenderer *pRenderer;
    VkRenderPass renderPass{};
    VkFramebuffer framebuffer{};
    VkPipeline pipeline{};
    UT_GraphicsPipelinePSOs pso{};
    VkDescriptorSet sets[2]{};
};


LLVK_NAMESPACE_END


#endif //DUALPASS_H
