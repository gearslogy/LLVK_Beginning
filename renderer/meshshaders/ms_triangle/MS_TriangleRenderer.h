//
// Created by liuya on 6/9/2025.
//

#ifndef MS_TRIANGLERENDERER_H
#define MS_TRIANGLERENDERER_H


#include "LLVK_UT_Pipeline.hpp"
#include "VulkanRenderer.h"
#include "renderer/public/Helper.hpp"
LLVK_NAMESPACE_BEGIN
class MS_TriangleRenderer : public VulkanRenderer{
protected:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    void updateUBO();
private:
    HLP::MVP uboData{};
    HLP::FramedUBO uboFramedUBO{};
    HLP::FramedSet sets{};

    VkDescriptorPool descPool{};
    VkPipeline pipeline{};
    VkDescriptorSet descSet{};
    VkDescriptorSetLayout descSetLayout{};
    VkPipelineLayout pipelineLayout{};
    UT_GraphicsPipelinePSOs pso{};

    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT{ VK_NULL_HANDLE };
};
LLVK_NAMESPACE_END


#endif //MS_TRIANGLERENDERER_H
