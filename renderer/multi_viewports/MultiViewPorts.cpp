//
// Created by liuya on 4/8/2025.
//

#include "MultiViewPorts.h"
#include <LLVK_UT_VmaBuffer.hpp>
#include "renderer/public/UT_CustomRenderer.hpp"
#include "MultiViewPortsShadowPass.h"
LLVK_NAMESPACE_BEGIN
MultiViewPorts::MultiViewPorts()  = default;
MultiViewPorts::~MultiViewPorts()  = default;

void MultiViewPorts::prepare() {
    mainCamera.mPosition = glm::vec3(0.0f, 5.0f, 20.0f);
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    // Ready pool
    HLP::createSimpleDescPool(device, descPool);
    ubo.lightPos = {102.828, 46, 16,1.0};
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    shadowPass = std::make_unique<MultiViewPortsShadowPass>(this);
    shadowPass->setLightPosition(ubo.lightPos);
    shadowPass->prepareRenderPass();
    shadowPass->prepareFramebuffer();
    shadowPass->prepareUBO();
    loadGeometry();
    prepareUBOs();
    prepareDescriptorSets(); // need shadow pass UBO, shadow MVP binding at : 3
    preparePipeline();
    shadowPass->preparePipeline(pipelineLayout);
}
void MultiViewPorts::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_resources(grid, treeTrunk, treeLeaves);
    UT_Fn::cleanup_descriptor_pool(device, descPool);
    UT_Fn::cleanup_pipeline(device, pipeline);
    UT_Fn::cleanup_resources(geomManager);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_descriptor_set_layout(device, setLayout);
    UT_Fn::cleanup_range_resources(uboBuffers);
    // shadow clear
    shadowPass->cleanup();
}


void MultiViewPorts::loadGeometry() {
    namespace fs = std::filesystem;
    const fs::path ROOT = "content/scene/multi_viewports";
    fs::path gltfRoot = ROOT/"gltf";
    fs::path texRoot = ROOT/"textures";

    setRequiredObjectsByRenderer(this, geomManager);
    GLTFLoaderV2::CustomAttribLoader<Geometry::vertex_t> geoAttribSet;
    grid.name = "grid";
    treeTrunk.name = "treeTrunk";
    treeLeaves.name = "treeLeaves";

    grid.geoLoader.load(gltfRoot/"grid.gltf", geoAttribSet);
    treeTrunk.geoLoader.load(gltfRoot/"trunk_cluster.gltf", geoAttribSet);
    treeLeaves.geoLoader.load(gltfRoot/"leaves_cluster.gltf", geoAttribSet);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(grid.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(treeTrunk.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(treeLeaves.geoLoader,geomManager);


    setRequiredObjectsByRenderer(this, grid.diff, grid.nrm);
    setRequiredObjectsByRenderer(this, treeTrunk.diff, treeTrunk.nrm);
    setRequiredObjectsByRenderer(this, treeLeaves.diff, treeLeaves.nrm);
    grid.diff.create(texRoot/"grid_D.jpg", colorSampler);
    grid.nrm.create(texRoot/"grid_NR.png", colorSampler,VK_FORMAT_R8G8B8A8_UNORM);

    treeLeaves.diff.create(texRoot/"leaves_D.png", colorSampler);
    treeLeaves.nrm.create(texRoot/"leaves_NRS.png", colorSampler,VK_FORMAT_R8G8B8A8_UNORM);

    treeTrunk.diff.create(texRoot/"trunk_D.png", colorSampler);
    treeTrunk.nrm.create(texRoot/"trunk_NR.TGA", colorSampler,VK_FORMAT_R8G8B8A8_UNORM);

}

void MultiViewPorts::prepareUBOs() {
    setRequiredObjectsByRenderer(this, uboBuffers);
    for (auto & uboBuffer : uboBuffers) {
        uboBuffer.createAndMapping(sizeof(UBO));
    }
    updateUBOs();
}
void MultiViewPorts::updateUBOs() {
    // left is perspective view
    auto [width, height] =  getSwapChainExtent();
    mainCamera.mAspect = (static_cast<float>(width)/2 ) / static_cast<float>(height);
    //std::cout << "aspect:" <<mainCamera.mAspect << std::endl;
    auto &proj0 = ubo.proj[1];
    auto &view0 = ubo.view[1];
    auto &camPos0 = ubo.camPos[1];
    proj0 = mainCamera.projection();
    proj0[1][1] *= -1;
    view0 = glm::mat4(1.0f) * mainCamera.view();
    camPos0 = glm::vec4{mainCamera.mPosition,1};

    // --- RIGHT VIEW (Top View, Orthographic) ---
    auto &proj1 = ubo.proj[0];
    auto &view1 = ubo.view[0];
    auto &camPos1 = ubo.camPos[0];
    // Orthographic projection (adjust bounds as needed)
    float halfWidth = 10.0f;  // Example: covers -10 to 10 in X and Z
    float halfHeight = halfWidth * (height / (width / 2.0f));  // Maintain aspect ratio
    proj1 = glm::ortho(
        -halfWidth, halfWidth,   // Left, Right
        -halfHeight, halfHeight, // Bottom, Top
        0.1f, 100.0f             // Near, Far
    );
    proj1[1][1] *= -1;
    camPos1 = glm::vec4{0, 20, 0, 1.0};
    view1 = glm::lookAt(glm::vec3(camPos1),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 1));
    for (auto & uboBuffer : uboBuffers) {
        memcpy(uboBuffer.mapped, &ubo, sizeof(UBO));
    }
}


void MultiViewPorts::prepareDescriptorSets() {
    // 1 set
    // binding=0 UBO
    // binding=1 diff
    // binding=2 NRM
    const auto &device = mainDevice.logicalDevice;
    using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO, // GeometryShader usage binding=0
        MetaDesc::CIS,
        MetaDesc::CIS,
        MetaDesc::UBO,                                     // ShadowUBO
        MetaDesc::CIS>;                                    // Shadow Texture Sampler
    using descPos = MetaDesc::desc_binding_position_t<0,1,2,3,4>;
    using descBindingUsage = MetaDesc::desc_binding_usage_t<
        VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,                     // Bind Shadow UBO
        VK_SHADER_STAGE_FRAGMENT_BIT>;                                                 // Shadow Texture
    constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
    const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
    if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&setLayout) != VK_SUCCESS) throw std::runtime_error("error create set0 layout");

    std::array<VkDescriptorSetLayout,2> layouts = {setLayout, setLayout}; // must be two, because we USE MAX_FLIGHT_FRAME
    auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
    UT_Fn::invoke_and_check("create scene tree sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, treeLeaves.sets.data());
    UT_Fn::invoke_and_check("create scene tree sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, treeTrunk.sets.data());
    UT_Fn::invoke_and_check("create scene grid sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, grid.sets.data());
    // update sets
    namespace FnDesc = FnDescriptor;
    auto writeSets = [&device, this](const Geometry &geo) {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet, 5> writes = {
                FnDesc::writeDescriptorSet(geo.sets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),
                FnDesc::writeDescriptorSet(geo.sets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &geo.diff.descImageInfo),
                FnDesc::writeDescriptorSet(geo.sets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2, &geo.nrm.descImageInfo),
                FnDesc::writeDescriptorSet(geo.sets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,3, &shadowPass->uboBuffers[i].descBufferInfo), // depth mvp
                FnDesc::writeDescriptorSet(geo.sets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4, &shadowPass->shadowFramebuffer.depthAttachment.descImageInfo)// depth attachment
            };
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }
    };
    writeSets(grid);
    writeSets(treeTrunk);
    writeSets(treeLeaves);

    // 4. create pipeline layout and inject push constant .
    const std::array offscreenSetLayouts{setLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(offscreenSetLayouts); // ONLY ONE SET
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = push_constant_stage;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(HLP::xform);
    pipelineLayoutCIO.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutCIO.pushConstantRangeCount =1 ;
    UT_Fn::invoke_and_check("ERROR create offscreen pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );
}


void MultiViewPorts::preparePipeline() {
    const auto &device = mainDevice.logicalDevice;
    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/multi_viewports_vert.spv",  device);
    const auto gsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/multi_viewports_geom.spv",  device);
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/multi_viewports_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);
    VkPipelineShaderStageCreateInfo gsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, gsMD);
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, gsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(getMainRenderPass());
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(HLP::VTXAttrib::VTXFmt_P_N_T_UV0_BindingsDesc,
        HLP::VTXAttrib::VTXFmt_P_N_T_UV0_AttribsDesc);
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,gsMD,fsMD);
}


void MultiViewPorts::render() {
    shadowPass->updateUBO();
    updateUBOs();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}

void MultiViewPorts::drawObjects() const {
    const auto &cmdBuf = getMainCommandBuffer();
    // geo render
    auto renderGeo = [&cmdBuf](auto &geo) {
        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &geo.verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,geo.indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuf, geo.indices.size(), 1, 0, 0, 0);
    };

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &grid.sets[currentFlightFrame], 0 , nullptr);
    vkCmdPushConstants(cmdBuf, pipelineLayout, push_constant_stage, 0, sizeof(HLP::xform), &grid.xform);
    renderGeo(grid.geoLoader.parts[0]);

    // render tree trunk
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &treeTrunk.sets[currentFlightFrame], 0 , nullptr);
    vkCmdPushConstants(cmdBuf, pipelineLayout, push_constant_stage, 0, sizeof(HLP::xform),&treeTrunk.xform);
    renderGeo(treeTrunk.geoLoader.parts[0]);
    // render tree leaves
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &treeLeaves.sets[currentFlightFrame], 0 , nullptr);
    vkCmdPushConstants(cmdBuf, pipelineLayout, push_constant_stage, 0, sizeof(HLP::xform),&treeLeaves.xform);
    renderGeo(treeLeaves.geoLoader.parts[0]);

}


void MultiViewPorts::recordCommandBuffer() {
    const auto &cmdBuf = getMainCommandBuffer();
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    auto [width, height] = getSwapChainExtent();

    // clear
    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    //<0>------------- render depth -----------



    const auto renderPassBeginInfo= FnCommand::renderPassBeginInfo(getMainFramebuffer(),
        simplePass.pass,
        getSwapChainExtent(),
        clearValues);
    UT_Fn::invoke_and_check("Rendering Error", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);

    shadowPass->recordCommandBuffer();

    // set viewports  0 = right(TOP), 1 = left(perspective)
    VkViewport viewports[2]{};
    VkRect2D   scissors[2]{};

    viewports[0] = FnCommand::viewport(width/2, height, width/2, 0); // right TOP view
    viewports[1] = FnCommand::viewport(width/2, height, 0, 0);       // left perspective view

    scissors[0] = FnCommand::scissor(width/2, height, width/2, 0); // right TOP view
    scissors[1] = FnCommand::scissor(width/2, height, 0, 0);       // left perspective view

    //<1> ------------   render secene ------
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(cmdBuf, 0, 2, viewports );
    vkCmdSetScissor(cmdBuf,0, 2, scissors);
    // ---------------------------  RENDER PASS ---------------------------
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    drawObjects();
    // ----------------------------
    vkCmdEndRenderPass(cmdBuf);
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}



LLVK_NAMESPACE_END