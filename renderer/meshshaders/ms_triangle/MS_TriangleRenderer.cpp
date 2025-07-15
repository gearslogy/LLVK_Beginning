//
// Created by liuya on 6/9/2025.
//

#include "MS_TriangleRenderer.h"
#include "renderer/public/Helper.hpp"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
void MS_TriangleRenderer::cleanupObjects(){
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    UT_Fn::cleanup_descriptor_pool(device, descPool);
    UT_Fn::cleanup_range_resources(uboFramedUBO);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_pipeline(device, pipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);

}


void MS_TriangleRenderer::prepare(){
    mainCamera.mPosition = glm::vec3(0, 0, 4);
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT"));
    // 0 : create desc pool
    HLP::createSimpleDescPool(device, descPool);
    // 1: ubo create
    setRequiredObjectsByRenderer(this, uboFramedUBO);
    for (auto &ubo: uboFramedUBO) {
        ubo.createAndMapping(sizeof(HLP::MVP));
    }
    updateUBO();

    // 2: set layout
    using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO>; // MVP
    using descPos = MetaDesc::desc_binding_position_t<0>;
    using descBindingUsage = MetaDesc::desc_binding_usage_t< VK_SHADER_STAGE_MESH_BIT_EXT>; // MVP
    constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
    const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
    if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&descSetLayout) != VK_SUCCESS) throw std::runtime_error("error create set0 layout");
    // 3: set
    std::array<VkDescriptorSetLayout,2> layouts = {descSetLayout, descSetLayout}; // must be two, because we USE MAX_FLIGHT_FRAME
    auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
    UT_Fn::invoke_and_check("create scene sets-0 error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, sets.data());
    // 4: update set
    namespace FnDesc = FnDescriptor;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::array<VkWriteDescriptorSet,1> writes = {
            FnDesc::writeDescriptorSet(sets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboFramedUBO[i].descBufferInfo),
        };
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
    }
    // 5: pipeline layout
    const std::array setLayouts{descSetLayout}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout, device, &pipelineLayoutCIO,nullptr, &pipelineLayout );
    // 6: pipeline
    createPipeline();
}

void MS_TriangleRenderer::updateUBO() {
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    uboData.model = glm::mat4(1.0f);
    memcpy(uboFramedUBO[getCurrentFlightFrame()].mapped, &uboData, sizeof(uboData));
}
void MS_TriangleRenderer::createPipeline() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    // pipeline
    const auto meshMD = FnPipeline::createShaderModuleFromSpvFile("shaders/ms_triangle_mesh.spv",  device);    //shader modules
    const auto taskMD = FnPipeline::createShaderModuleFromSpvFile("shaders/ms_triangle_task.spv",  device);
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/ms_triangle_frag.spv",  device);
    VkPipelineShaderStageCreateInfo meshMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_MESH_BIT_EXT, meshMD);    //shader stages
    VkPipelineShaderStageCreateInfo taskMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_TASK_BIT_EXT, taskMD);
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);

    pso.setShaderStages(taskMD_ssCIO, meshMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(getMainRenderPass());
    pso.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
    //pso.vertexInputStageCIO.pVertexAttributeDescriptions = nullptr;
    //pso.vertexInputStageCIO.pVertexBindingDescriptions = nullptr;
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo();

    UT_GraphicsPipelinePSOs::createPipeline(device, pso, getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,meshMD,taskMD,fsMD);
}



void MS_TriangleRenderer::render(){
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    const auto &cmdBuf = getMainCommandBuffer();

    // 0 : update UBO
    updateUBO();
    // 1 : Record command buffer info
    VkCommandBufferBeginInfo beginInfo = FnCommand::commandBufferBeginInfo();
    auto [width, height] = getSwapChainExtent();
    // clear
    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    const auto renderPassBeginInfo= FnCommand::renderPassBeginInfo(getMainFramebuffer(),
      simplePass.pass,
      getSwapChainExtent(),
      clearValues);
    const auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    const auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );


    vkBeginCommandBuffer(cmdBuf, &beginInfo );
    {
        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf,0, 1, &scissor);
        // ---------------------------  RENDER PASS ---------------------------
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &sets[currentFlightFrame], 0 , nullptr);
        // RECORD COMMAND BUFFER
        vkCmdDrawMeshTasksEXT(cmdBuf, 1, 1, 1); // 运行 1 个任务着色器工作组
        /*
         * 1:
         * vkCmdDrawMeshTasksEXT(commandBuffer, 3, 1, 1); // 相当于 EmitMeshTasksEXT(3, 1, 1)
         这样就不用task shader了。我们当前task shader是：
            #version 460 core
            #extension GL_EXT_mesh_shader : require
            void main(){
                EmitMeshTasksEXT(3,1,1); // 该任务着色器分发 3 个网格工作组,	3 个网格着色器运行
            }
         2: 所以说task shader不一定非得要存在。
         */
        vkCmdEndRenderPass(cmdBuf);
    }
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );

    // 2: submit command buffer
    submitMainCommandBuffer();
    // 3 : present
    presentMainCommandBufferFrame();

}


LLVK_NAMESPACE_END