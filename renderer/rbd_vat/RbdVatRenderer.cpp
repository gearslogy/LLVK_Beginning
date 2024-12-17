//
// Created by liuya on 12/8/2024.
//

#include "RbdVatRenderer.h"

#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_VmaBuffer.hpp>
#include <Pipeline.hpp>

#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
RbdVatRenderer::RbdVatRenderer()  = default;

void RbdVatRenderer::prepare() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    setRequiredObjectsByRenderer(this, geomManager);
    setRequiredObjectsByRenderer(this, texPosition,texOrient, texDiff);
    auto fracture_index_loader = GLTFLoaderV2::CustomAttribLoader<GLTFVertexVATFracture, uint32_t>{"_fracture_index"};
    // 1.geo
    buildings.load("content/scene/rbd_vat/gltf/buildings.gltf", std::move(fracture_index_loader));
    UT_VmaBuffer::addGeometryToSimpleBufferManager(buildings,geomManager);
    // 2.samplers
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    vatSampler = FnImage::createExrVATSampler(device);
    // 3.tex
    texDiff.create("content/scene/rbd_vat/resources/gpu_textures/39_MedBuilding_gpu_D.ktx2", colorSampler);
    texPosition.create("content/scene/vat_tex/position.ktx2",vatSampler);
    texOrient.create("content/scene/vat_tex/orient.ktx2",vatSampler);

    // desc pool
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR"};

    prepareUBO();
    prepareDescSets();
    preparePipeline();
}

void RbdVatRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    UT_Fn::cleanup_resources(geomManager, texDiff, texPosition, texOrient);
    UT_Fn::cleanup_sampler(device, colorSampler, vatSampler);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_pipeline(device, scenePipeline);
}

void RbdVatRenderer::prepareDescSets() {
    const auto &device = mainDevice.logicalDevice;

    {//set0 desc
        using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO>; // MVP
        using descPos = MetaDesc::desc_binding_position_t<0>;
        using descBindingUsage = MetaDesc::desc_binding_usage_t< VK_SHADER_STAGE_VERTEX_BIT>; // MVP
        constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
        const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
        if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&descSetLayout_set0) != VK_SUCCESS) throw std::runtime_error("error create set0 layout");

        std::array<VkDescriptorSetLayout,2> layouts = {descSetLayout_set0, descSetLayout_set0}; // must be two, because we USE MAX_FLIGHT_FRAME
        auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
        UT_Fn::invoke_and_check("create scene sets-0 error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, descSets_set0.data());
        // update sets
        namespace FnDesc = FnDescriptor;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet, 1> writes = {
                FnDesc::writeDescriptorSet(descSets_set0[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),          // scene model_view_proj_instance , used in VS shader
            };
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }

    }
    {//set1 desc
        using descTypes = MetaDesc::desc_types_t<MetaDesc::CIS, MetaDesc::CIS, MetaDesc::CIS>; // base/vat
        using descPos = MetaDesc::desc_binding_position_t<0,1,2>;
        using descBindingUsage = MetaDesc::desc_binding_usage_t<
            VK_SHADER_STAGE_FRAGMENT_BIT, // base color tex
            VK_SHADER_STAGE_FRAGMENT_BIT,  // VAT tex
            VK_SHADER_STAGE_FRAGMENT_BIT  // VAT tex
        >;
        constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
        const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
        if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&descSetLayout_set1) != VK_SUCCESS) throw std::runtime_error("error create set1 layout");

        std::array<VkDescriptorSetLayout,2> layouts = {descSetLayout_set1, descSetLayout_set1}; // must be two, because we USE MAX_FLIGHT_FRAME
        auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
        UT_Fn::invoke_and_check("create scene sets-1 error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, descSets_set1.data());
        // update sets
        namespace FnDesc = FnDescriptor;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet,3> writes = {
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDiff.descImageInfo),          // scene model_view_proj_instance , used in VS shader
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texPosition.descImageInfo),          // scene model_view_proj_instance , used in VS shader
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texOrient.descImageInfo)          // scene model_view_proj_instance , used in VS shader
            };
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }
    }


    // pipeline layout
    const std::array setLayouts{descSetLayout_set0, descSetLayout_set1}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );

}
void RbdVatRenderer::prepareUBO() {
    setRequiredObjectsByRenderer(this, uboBuffers);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uboData));
}
void RbdVatRenderer::render() {

}
void RbdVatRenderer::preparePipeline() {
    const auto &device = mainDevice.logicalDevice;
    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/csm_scene_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/csm_scene_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(getMainRenderPass());
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, getPipelineCache(), scenePipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}
void RbdVatRenderer::updateUBO() {
    auto [width, height] =   getSwapChainExtent();
    auto &&mainCamera = getMainCamera();
    const auto frame = getCurrentFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    uboData.model = glm::mat4(1.0f);
    uboData.frame = ?;
    memcpy(uboBuffers[frame].mapped, &uboData, sizeof(uboData));
}


auto enable_options = [](auto &option , auto ... name) {

};




LLVK_NAMESPACE_END