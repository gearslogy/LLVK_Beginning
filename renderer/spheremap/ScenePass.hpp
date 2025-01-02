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

LLVK_NAMESPACE_BEGIN
namespace SPHEREMAP_NAMESPACE {


template<typename renderer_t, typename geo_loader_t>
struct ScenePass {
    renderer_t *pRenderer{}; // to bind
    VmaUBOExrRGBATexture *pCubeTex{}; // to bind
    geo_loader_t mLoader;  // to load geometry
    void prepare();
    void recordCommandBuffer(const VkCommandBuffer &cmdBuf);
    void cleanup();
private:
    UT_GraphicsPipelinePSOs pso;
    VmaSimpleGeometryBufferManager geomManager{};
    struct {
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;

        glm::mat4 invView;

        // glm::mat4 invViewModel;
        // Notice that the next two are the same:
        // glm::mat4 inverse1 = glm::inverse(view * model);
        // glm::mat4 inverse2 = glm::inverse(model) * glm::inverse(view);
    }uboData;
    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descSets{};
    VkDescriptorSetLayout setLayout{}; // for set=0
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
};


template<typename renderer_t, typename geo_loader_t>
void ScenePass<renderer_t, geo_loader_t>::prepare() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &phyDevice = pRenderer->getMainDevice().physicalDevice;

    // geo
    setRequiredObjectsByRenderer(pRenderer, geomManager);
    GLTFLoaderV2::CustomAttribLoader<VTXFmt_P_N> attribSet;
    mLoader.load("content/scene/spheremap/reflect_sphere.gltf", attribSet);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(mLoader,geomManager);

    setRequiredObjectsByRenderer(pRenderer, uboBuffers);
    for (auto &ubo: uboBuffers) {
        ubo.createAndMapping(sizeof(uboData));
    }
    // set layout
    const auto &descPool = pRenderer->descPool;
    using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO, MetaDesc::CIS>; // MVP
    using descPos = MetaDesc::desc_binding_position_t<0,1>;
    using descBindingUsage = MetaDesc::desc_binding_usage_t< VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT>; // MVP
    constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
    const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
    if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&setLayout) != VK_SUCCESS) throw std::runtime_error("error create set layout");
    // allocate sets
    std::array<VkDescriptorSetLayout,2> layouts = {setLayout, setLayout};
    auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
    UT_Fn::invoke_and_check("create scene sets-0 error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, descSets.data());
    // update sets
    namespace FnDesc = FnDescriptor;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::array<VkWriteDescriptorSet, 2> writes = {
            FnDesc::writeDescriptorSet(descSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),
            FnDesc::writeDescriptorSet(descSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 , &pCubeTex->descImageInfo)
        };
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
    }
    // pipelineLayout and pipeline
    const std::array setLayouts{setLayout}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );

    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/spheremap_scene_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/spheremap_scene_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(pRenderer->getMainRenderPass());

    std::array<VkVertexInputAttributeDescription,2> attribsDesc{};
    attribsDesc[0] = { 0,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N, P)};
    attribsDesc[1] = { 1,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N, N)};
    VkVertexInputBindingDescription vertexBinding{0, sizeof(VTXFmt_P_N), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array bindingsDesc{vertexBinding};
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}


template<typename renderer_t, typename geo_loader_t>
void ScenePass<renderer_t, geo_loader_t>::recordCommandBuffer(const VkCommandBuffer &cmdBuf) {
    // update ubo
    auto [width, height] =   pRenderer->getSwapChainExtent();
    auto &&mainCamera = pRenderer->getMainCamera();
    const auto frame = pRenderer->getCurrentFlightFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    uboData.model = glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.0f, -2.0f});
    uboData.invView = glm::inverse(uboData.view);

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
void ScenePass<renderer_t, geo_loader_t>::cleanup() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &phyDevice = pRenderer->getMainDevice().physicalDevice;
    UT_Fn::cleanup_resources(geomManager);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_pipeline(device, pipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, setLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
}
}

LLVK_NAMESPACE_END



