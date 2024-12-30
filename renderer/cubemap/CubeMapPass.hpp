//
// Created by liuya on 12/28/2024.
//

#ifndef CUBEMAPPASS_H
#define CUBEMAPPASS_H


#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_Pipeline.hpp>
#include <LLVK_UT_VmaBuffer.hpp>
#include "LLVK_SYS.hpp"
#include "LLVK_VmaBuffer.h"
#include "renderer/public/CustomVertexFormat.hpp"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
template<typename renderer_t, typename geo_loader_t>
struct CubeMapPass {
    renderer_t *pRenderer; // to bind
    geo_loader_t mLoader;  // to load geometry
    VmaUBOKTX2Texture mCubeTex; // to bind texture
    VkSampler mCubeTexSampler{};
    void prepare();
    void recordCommandBuffer(const VkCommandBuffer &cmdBuf);
    void cleanup();
private:
    UT_GraphicsPipelinePSOs pso;
    VmaSimpleGeometryBufferManager geomManager{};
    VkSampler mSampler{};

    struct {
        glm::mat4 proj;
        glm::mat4 view;
    }uboData;
    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descSets{};
    VkDescriptorSetLayout setLayout{}; // for set=0
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};

};

template<typename renderer_t,typename geo_loader_t>
void CubeMapPass<renderer_t,geo_loader_t>::prepare() {
    const auto &device = pRenderer->mainDevice.logicalDevice;
    // sampler create
    auto samplerCIO = FnImage::samplerCreateInfo();
    samplerCIO.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCIO.addressModeV = samplerCIO.addressModeU;
    samplerCIO.addressModeW = samplerCIO.addressModeU;
    samplerCIO.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    if (vkCreateSampler(device, &samplerCIO, nullptr, &mCubeTexSampler)!= VK_SUCCESS ) throw std::runtime_error("failed to create sampler");
    // geo
    setRequiredObjectsByRenderer(pRenderer, geomManager);
    mLoader.load("content/scene/cubemap/gltf/cube.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(mLoader,geomManager);
    // load cube tex
    setRequiredObjectsByRenderer(pRenderer, mCubeTex);
    mCubeTex.create("content/scene/cubemap/tex/cubemap.ktx2", mCubeTexSampler);
    // ubo
    setRequiredObjectsByRenderer(this, uboBuffers);
    for (auto &ubo: uboBuffers) {
        ubo.createAndMapping(sizeof(uboData));
    }
    // set layout
    const auto &descPool = pRenderer->descPool;
    using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO, MetaDesc::CIS>; // MVP
    using descPos = MetaDesc::desc_binding_position_t<0,1>;
    using descBindingUsage = MetaDesc::desc_binding_usage_t< VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT>; // MVP
    constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
    const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
    if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&setLayout) != VK_SUCCESS) throw std::runtime_error("error create set layout");
    // allocate sets
    std::array<VkDescriptorSetLayout,2> layouts = {setLayout, setLayout}; // must be two, because we USE MAX_FLIGHT_FRAME
    auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
    UT_Fn::invoke_and_check("create scene sets-0 error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, descSets.data());
    // update sets
    namespace FnDesc = FnDescriptor;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::array<VkWriteDescriptorSet, 2> writes = {
            FnDesc::writeDescriptorSet(descSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),          // scene model_view_proj_instance , used in VS shader
            FnDesc::writeDescriptorSet(descSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 &mCubeTex.descImageInfo),          // scene model_view_proj_instance , used in VS shader
        };
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
    }
    // pipelineLayout and pipeline
    const std::array setLayouts{setLayout}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );

    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/rbd_vat_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/rbd_vat_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(pRenderer->getMainRenderPass());
    pso.depthStencilStateCIO.depthTestEnable = VK_TRUE;
    pso.depthStencilStateCIO.depthWriteEnable = VK_FALSE;
    pso.depthStencilStateCIO.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    std::array<VkVertexInputAttributeDescription,1> attribsDesc{};
    attribsDesc[0] = { 0,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P, P)};
    VkVertexInputBindingDescription vertexBinding{0, sizeof(VTXFmt_P), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array bindingsDesc{vertexBinding};
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}


template<typename renderer_t,typename geo_loader_t>
void CubeMapPass<renderer_t,geo_loader_t>::recordCommandBuffer(const VkCommandBuffer &cmdBuf) {
    // update ubo
    auto [width, height] =   pRenderer->getSwapChainExtent();
    auto &&mainCamera = pRenderer->getMainCamera();
    const auto frame = pRenderer->getCurrentFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    memcpy(uboBuffers[frame].mapped, &uboData, sizeof(uboData));
    //
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS ,pipeline);
    auto viewport = FnCommand::viewport(width, height );
    auto scissor = FnCommand::scissor(width, height );
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
         0, 1, descSets[pRenderer->getCurrentFlightFrame()], 0, nullptr);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &mLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(cmdBuf,mLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuf, mLoader.parts[0].indices.size(), 1, 0, 0, 0);
}

template<typename renderer_t, typename geo_loader_t>
void CubeMapPass<renderer_t, geo_loader_t>::cleanup() {
    const auto &device = pRenderer->mainDevice.logicalDevice;
    const auto &phyDevice = pRenderer->mainDevice.physicalDevice;
    UT_Fn::cleanup_resources(geomManager, mCubeTex);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_sampler(device, mSampler);
    UT_Fn::cleanup_pipeline(device, pipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, setLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_sampler(device, mCubeTexSampler);
}


LLVK_NAMESPACE_END


#endif //CUBEMAPPASS_H
