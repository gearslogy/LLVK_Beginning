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
struct RenderContainerOneSet;
// our first render pass that rendering head and grid
struct OpaqueScenePass {
    explicit OpaqueScenePass(DualPassRenderer *renderer);
    ~OpaqueScenePass();
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
    VkRenderPass opaqueRenderPass{};
    VkFramebuffer opaqueFB{};
    VkPipeline opaquePipeline{};
    UT_GraphicsPipelinePSOs pso{};

    std::unique_ptr<RenderContainerOneSet> renderContainer;


};


LLVK_NAMESPACE_END


#endif //DUALPASS_H
