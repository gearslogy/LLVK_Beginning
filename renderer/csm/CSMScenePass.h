//
// Created by liuya on 11/10/2024.
//

#ifndef CSMSCENEPASS_H
#define CSMSCENEPASS_H

#include <LLVK_UT_Pipeline.hpp>

#include "LLVK_SYS.hpp"

LLVK_NAMESPACE_BEGIN
class CSMRenderer;
struct CSMScenePass {
    explicit CSMScenePass(CSMRenderer *rd);
    void prepare();
    void cleanup();
    void recordCommandBuffer();


private:
    CSMRenderer* pRenderer;
    VkPipelineLayout pipelineLayout{};
    VkPipeline instancePipeline{};
    VkPipeline normalPipeline{};
    UT_GraphicsPipelinePSOs pso;

};

LLVK_NAMESPACE_END


#endif //CSMSCENEPASS_H
