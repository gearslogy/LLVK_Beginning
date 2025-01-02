//
// Created by liuya on 1/1/2025.
//

#pragma once


#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_Pipeline.hpp>
#include <LLVK_UT_VmaBuffer.hpp>
#include "LLVK_SYS.hpp"
#include "LLVK_VmaBuffer.h"
#include "renderer/public/CustomVertexFormat.hpp"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "LLVK_ExrImage.h"
LLVK_NAMESPACE_BEGIN
namespace SPHEREMAP_NAMESPACE {
template<typename renderer_t, typename geo_loader_t>
struct SphereMapPass {
    renderer_t *pRenderer; // to bind
    geo_loader_t mLoader;  // to load geometry
    VmaUBOExrRGBATexture mSphereTex; // to bind texture
    VkSampler mSphereTexSampler{};
    void prepare();
    void recordCommandBuffer(const VkCommandBuffer &cmdBuf);
    void cleanup();
private:
    UT_GraphicsPipelinePSOs pso;
    VmaSimpleGeometryBufferManager geomManager{};


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
void SphereMapPass<renderer_t,geo_loader_t>::prepare() {
    //auto &&cam = pRenderer->getMainCamera();
    //cam.mPosition = glm::vec3(0.0f);
    //cam.updateCameraVectors();
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    // sampler create
    auto samplerCIO = FnImage::samplerCreateInfo();
    samplerCIO.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCIO.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCIO.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCIO.maxAnisotropy = 16;
    //samplerCIO.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    if (vkCreateSampler(device, &samplerCIO, nullptr, &mSphereTexSampler)!= VK_SUCCESS ) throw std::runtime_error("failed to create sampler");
    // geo
    setRequiredObjectsByRenderer(pRenderer, geomManager);
    mLoader.load("content/scene/spheremap/sky_sphere.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(mLoader,geomManager);
    // load cube tex
    setRequiredObjectsByRenderer(pRenderer, mSphereTex);
    mSphereTex.create("content/scene/spheremap/kloppenheim_06_puresky_2k.exr", mSphereTexSampler);
    // ubo
    setRequiredObjectsByRenderer(pRenderer, uboBuffers);
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
            FnDesc::writeDescriptorSet(descSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 , &mSphereTex.descImageInfo),          // scene model_view_proj_instance , used in VS shader
        };
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
    }
    // pipelineLayout and pipeline
    const std::array setLayouts{setLayout}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );

    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/spheremap_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/spheremap_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(pRenderer->getMainRenderPass());
    pso.depthStencilStateCIO.depthTestEnable = VK_TRUE;
    pso.depthStencilStateCIO.depthWriteEnable = VK_FALSE;
    pso.depthStencilStateCIO.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pso.rasterizerStateCIO.cullMode = VK_CULL_MODE_FRONT_BIT;
    std::array<VkVertexInputAttributeDescription,1> attribsDesc{};
    attribsDesc[0] = { 0,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P, P)};
    VkVertexInputBindingDescription vertexBinding{0, sizeof(VTXFmt_P), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array bindingsDesc{vertexBinding};
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}


template<typename renderer_t,typename geo_loader_t>
void SphereMapPass<renderer_t,geo_loader_t>::recordCommandBuffer(const VkCommandBuffer &cmdBuf) {
    // update ubo
    auto [width, height] =   pRenderer->getSwapChainExtent();
    auto &&mainCamera = pRenderer->getMainCamera();
    const auto frame = pRenderer->getCurrentFlightFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = glm::mat4{glm::mat3{mainCamera.view()} };
    memcpy(uboBuffers[frame].mapped, &uboData, sizeof(uboData));
    //
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS ,pipeline);
    auto viewport = FnCommand::viewport(width, height );
    auto scissor = FnCommand::scissor(width, height );
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
         0, 1, &descSets[pRenderer->getCurrentFlightFrame()], 0, nullptr);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &mLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(cmdBuf,mLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuf, mLoader.parts[0].indices.size(), 1, 0, 0, 0);
}

template<typename renderer_t, typename geo_loader_t>
void SphereMapPass<renderer_t, geo_loader_t>::cleanup() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &phyDevice = pRenderer->getMainDevice().physicalDevice;
    UT_Fn::cleanup_resources(geomManager, mSphereTex);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_pipeline(device, pipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, setLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_sampler(device, mSphereTexSampler);
}
}
LLVK_NAMESPACE_END
