//
// Created by liuya on 6/9/2025.
//

#ifndef MS_TRIANGLERENDERER_H
#define MS_TRIANGLERENDERER_H


#include "LLVK_UT_Pipeline.hpp"
#include "VulkanRenderer.h"
LLVK_NAMESPACE_BEGIN
class MS_TriangleRenderer : public VulkanRenderer{
protected:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;

private:
    VkDescriptorPool descPool{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
    UT_GraphicsPipelinePSOs pso{};

};
LLVK_NAMESPACE_END


#endif //MS_TRIANGLERENDERER_H
