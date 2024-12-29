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
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
template<typename renderer_t, typename geo_loader_t>
struct CubeMapPass {
    renderer_t *pRenderer; // to bind
    geo_loader_t mLoader;  // to load geometry
    VmaUBOKTX2Texture mCubeTex; // to bind texture
    void prepare();
    void recordCommandBuffer(VkCommandBuffer commandBuffer);
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
    // geo
    setRequiredObjectsByRenderer(pRenderer, geomManager);
    mLoader.load("content/scene/cubemap/gltf/cube.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(mLoader,geomManager);
    // load cube tex
    setRequiredObjectsByRenderer(pRenderer, mCubeTex);
    mCubeTex.create("content/scene/cubemap/tex/cubemap.ktx2");
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



    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);

}


template<typename renderer_t,typename geo_loader_t>
void CubeMapPass<renderer_t,geo_loader_t>::recordCommandBuffer(VkCommandBuffer commandBuffer) {
    // update ubo
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

}


LLVK_NAMESPACE_END


#endif //CUBEMAPPASS_H
