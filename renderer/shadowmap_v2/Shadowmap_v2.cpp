//
// Created by liuya on 9/1/2024.
//

#include "Shadowmap_v2.h"

#include <libs/magic_enum.hpp>

#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
#include "Pipeline.hpp"
LLVK_NAMESPACE_BEGIN
Shadowmap_v2::Shadowmap_v2() {
    mainCamera.mPosition = glm::vec3(15,20,192);
}

void Shadowmap_v2::render() {
    updateUniformBuffers();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}

void Shadowmap_v2::cleanupObjects() {
    auto device = mainDevice.logicalDevice;
    vkDestroyDescriptorPool(device, descPool, nullptr);
    UT_Fn::cleanup_resources(geoBufferManager, foliageTex, gridTex);
    UT_Fn::cleanup_sampler(device, colorSampler);


    // cleanup scene
    UT_Fn::cleanup_pipeline(device, scenePipeline.opacity, scenePipeline.opaque);
    UT_Fn::cleanup_descriptor_set_layout(device, sceneDescriptorSetLayout);
    UT_Fn::cleanup_pipeline_layout(device, scenePipelineLayout);

    // cleaun up uniform buffers
    uniformBuffers.offscreen.cleanup();
    uniformBuffers.scene.cleanup();
}

void Shadowmap_v2::prepare() {
    colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    loadTextures();
    loadModels();
    createOffscreenDepthAttachment();
    createOffscreenRenderPass();
    createOffscreenFramebuffer();
    prepareUniformBuffers();
    prepareDescriptorSets();
    preparePipelines();
}

void Shadowmap_v2::loadTextures() {
    setRequiredObjects(foliageTex, gridTex);
    foliageTex.create("content/plants/gardenplants/tex_array.ktx2", colorSampler );
    gridTex.create("content/shadowmap_v2/grid_tex/tex_array.ktx2",colorSampler);
}

void Shadowmap_v2::loadModels() {
    setRequiredObjects(geoBufferManager);
    gridGeo.load("content/shadowmap_v2/grid.gltf");
    foliageGeo.load("content/plants/gardenplants/var0.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(gridGeo, geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(foliageGeo, geoBufferManager);
}



void Shadowmap_v2::prepareUniformBuffers() {
    setRequiredObjects(uniformBuffers.scene);
    uniformBuffers.scene.createAndMapping(sizeof(uniformDataScene));
    lightPos = {281.654, 120,316.942};



    updateUniformBuffers();
}
void Shadowmap_v2::updateUniformBuffers() {
    // offscreen


    auto [width, height] = simpleSwapchain.swapChainExtent;
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uniformDataScene.projection = mainCamera.projection();
    uniformDataScene.projection[1][1] *= -1;
    uniformDataScene.view = mainCamera.view();
    uniformDataScene.model = glm::mat4(1.0f);
    uniformDataScene.depthBiasMVP = uniformDataOffscreen.depthMVP;

    memcpy(uniformBuffers.scene.mapped, &uniformDataScene, sizeof(uniformDataScene));
}

void Shadowmap_v2::prepareDescriptorSets() {
    const auto &device = mainDevice.logicalDevice;
    const auto &physicalDevice = mainDevice.physicalDevice;
    // cal the UBO count
    constexpr auto ubo_depth_map_gen_count = 2; // depth mvp opacity and opaque
    constexpr auto tex_depth_map_gen_opacity_count = 2; // sample opacity map to discard.

    constexpr auto scene_ubo_count = 2; //  opacity and opaque
    constexpr auto scene_opacity_tex_sampler2d_count = 2; // texture + depth
    constexpr auto scene_opaque_tex_sampler2d_count = 2;  // texture + depth

    constexpr auto UBO_COUNT = ubo_depth_map_gen_count + scene_ubo_count;
    constexpr auto TEX_COUNT = tex_depth_map_gen_opacity_count +
                               scene_opacity_tex_sampler2d_count +
                               scene_opaque_tex_sampler2d_count;

    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UBO_COUNT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TEX_COUNT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 4); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert(result == VK_SUCCESS);

    // we only have one set.
    auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);         // ubo
    auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // maps
    const std::array offscreen_setLayout_bindings = {set0_ubo_binding0, set0_ubo_binding1};
    const VkDescriptorSetLayoutCreateInfo offscreenSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(offscreen_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create offscreen descriptor set layout",vkCreateDescriptorSetLayout,device, &offscreenSetLayoutCIO, nullptr, &offscreenDescriptorSetLayout);
    std::array offscreen_setLayouts = {offscreenDescriptorSetLayout}; // only one set
    // create set
    auto offscreenAllocInfo = FnDescriptor::setAllocateInfo(descPool,offscreen_setLayouts);
    UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &offscreenAllocInfo,&offscreenSets.opacity );
    UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &offscreenAllocInfo,&offscreenSets.opaque );


    auto set0_ubo_binding2 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT); // depth
    const std::array scene_setLayout_bindings = {set0_ubo_binding0, set0_ubo_binding1, set0_ubo_binding2};
    const VkDescriptorSetLayoutCreateInfo sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(scene_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create offscreen descriptor set layout",vkCreateDescriptorSetLayout,device, &sceneSetLayoutCIO, nullptr, &sceneDescriptorSetLayout);
    auto sceneAllocInfo = FnDescriptor::setAllocateInfo(descPool,&sceneDescriptorSetLayout,1); // only one set
    UT_Fn::invoke_and_check("Error create scene sets",vkAllocateDescriptorSets,device, &sceneAllocInfo,&sceneSets.opacity );
    UT_Fn::invoke_and_check("Error create scene sets",vkAllocateDescriptorSets,device, &sceneAllocInfo,&sceneSets.opaque );
    // write set
    // - offscreen opacity
    std::vector<VkWriteDescriptorSet> opacityWriteSets;
    opacityWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(offscreenSets.opacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.offscreen.descBufferInfo));
    opacityWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(offscreenSets.opacity,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &foliageTex.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opacityWriteSets.size()),opacityWriteSets.data(),0, nullptr);
    // - offscreen opaque
    std::vector<VkWriteDescriptorSet> opaqueWriteSets;
    opaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(offscreenSets.opaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.offscreen.descBufferInfo));
    opaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(offscreenSets.opaque,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &gridTex.descImageInfo));// 实际这里我不想绑定。但是vulkan只要descriptorSetLayout 有这个bindign=1。就必须更新....
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opaqueWriteSets.size()),opaqueWriteSets.data(),0, nullptr);

    // - scene opacity
    std::vector<VkWriteDescriptorSet> opacitySceneWriteSets;
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.scene.descBufferInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &foliageTex.descImageInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2, &shadowFramebuffer.depthAttachment.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opacitySceneWriteSets.size()),opacitySceneWriteSets.data(),0, nullptr);

    // - scene opaque
    std::vector<VkWriteDescriptorSet> sceneOpaqueWriteSets;
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.scene.descBufferInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &gridTex.descImageInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2, &shadowFramebuffer.depthAttachment.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(sceneOpaqueWriteSets.size()),sceneOpaqueWriteSets.data(),0, nullptr);
}



void Shadowmap_v2::preparePipelines() {
    auto device = mainDevice.logicalDevice;
    const auto offscreenVertMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/offscreen_vert.spv",  device);
    const auto offscreenFragMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/offscreen_frag.spv",  device);

    const auto sceneVertMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/scene_vert.spv",  device);
    const auto sceneFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/scene_frag.spv",  device); // need VK_CULLING_NONE

    VkPipelineShaderStageCreateInfo offscreenVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, offscreenVertMoudule);
    VkPipelineShaderStageCreateInfo offscreenFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, offscreenFragMoudule);
    VkPipelineShaderStageCreateInfo sceneVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, sceneVertMoudule);
    VkPipelineShaderStageCreateInfo sceneFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, sceneFragModule);

    // 2. vertex input
    std::array bindings = {GLTFVertex::bindings()};
    auto attribs = GLTFVertex::attribs();
    VkPipelineVertexInputStateCreateInfo vertexInput_ST_CIO = FnPipeline::vertexInputStateCreateInfo(bindings, attribs);
    // 3. assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly_ST_CIO = FnPipeline::inputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, VK_FALSE);
    // 4 viewport and scissor
    VkPipelineViewportStateCreateInfo viewport_ST_CIO = FnPipeline::viewPortStateCreateInfo();
    // 5. dynamic state
    auto dynamicsStates = FnPipeline::simpleDynamicsStates();
    VkPipelineDynamicStateCreateInfo dynamics_ST_CIO= FnPipeline::dynamicStateCreateInfo(dynamicsStates);
    // 6. rasterization
    VkPipelineRasterizationStateCreateInfo rasterization_ST_CIO = FnPipeline::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    // 7. multisampling
    VkPipelineMultisampleStateCreateInfo multisample_ST_CIO=FnPipeline::multisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    // 8. blending
    std::array colorBlendAttamentState = {FnPipeline::simpleOpaqueColorBlendAttacmentState()};
    VkPipelineColorBlendStateCreateInfo blend_ST_CIO = FnPipeline::colorBlendStateCreateInfo(colorBlendAttamentState);
    // 9-1 offscreen pipeline layout
    const std::array offscreenSetLayouts{offscreenDescriptorSetLayout};
    VkPipelineLayoutCreateInfo offscreenSetLayout_CIO = FnPipeline::layoutCreateInfo(offscreenSetLayouts); // ONLY ONE SET
    constexpr uint32_t pushCount = 1;
    constexpr VkPushConstantRange pushRanges[pushCount] = {{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(float) } };
    offscreenSetLayout_CIO.pPushConstantRanges = pushRanges;
    offscreenSetLayout_CIO.pushConstantRangeCount = pushCount;
    UT_Fn::invoke_and_check("ERROR create offscreen pipeline layout",vkCreatePipelineLayout,device, &offscreenSetLayout_CIO,nullptr, &offscreenPipelineLayout );
    // 9-2 scene pipeline layout
    const std::array sceneSetLayouts{sceneDescriptorSetLayout};
    VkPipelineLayoutCreateInfo sceneSetLayout_CIO = FnPipeline::layoutCreateInfo(sceneSetLayouts);
    sceneSetLayout_CIO.pPushConstantRanges = pushRanges;
    sceneSetLayout_CIO.pushConstantRangeCount = pushCount;
    UT_Fn::invoke_and_check("ERROR create scene pipeline layout",vkCreatePipelineLayout,device, &sceneSetLayout_CIO,nullptr, &scenePipelineLayout );

    // 10
    VkPipelineDepthStencilStateCreateInfo ds_ST_CIO = FnPipeline::depthStencilStateCreateInfoEnabled();
    // 11. PIPELINE
    VkGraphicsPipelineCreateInfo pipeline_CIO = FnPipeline::pipelineCreateInfo();
    pipeline_CIO.stageCount = 2;
    pipeline_CIO.pVertexInputState = &vertexInput_ST_CIO;
    pipeline_CIO.pInputAssemblyState = &inputAssembly_ST_CIO;
    pipeline_CIO.pViewportState = &viewport_ST_CIO;
    pipeline_CIO.pDynamicState = &dynamics_ST_CIO;
    pipeline_CIO.pMultisampleState = &multisample_ST_CIO;
    pipeline_CIO.pDepthStencilState = &ds_ST_CIO;
    pipeline_CIO.subpass = 0; // ONLY USE ONE PASS
    pipeline_CIO.pColorBlendState = &blend_ST_CIO;

    // 11-1 create offscreen pipelines
    rasterization_ST_CIO.cullMode = VK_CULL_MODE_NONE; // Disable culling, so all faces contribute to shadows
    VkPipelineShaderStageCreateInfo offscreenStages[] = { offscreenVertShaderStageCreateInfo, offscreenFragShaderStageCreateInfo}; // only vertex
    pipeline_CIO.renderPass = shadowFramebuffer.renderPass;
    pipeline_CIO.layout = offscreenPipelineLayout;
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    pipeline_CIO.stageCount = 2;
    pipeline_CIO.pStages = offscreenStages;
    VkPipelineColorBlendStateCreateInfo emptyBlending_CIO = {};
    emptyBlending_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    emptyBlending_CIO.logicOpEnable = VK_FALSE;
    emptyBlending_CIO.attachmentCount = 0; // 无颜色附件
    pipeline_CIO.pColorBlendState = &emptyBlending_CIO; // offscreen阶段不许用
    UT_Fn::invoke_and_check( "error create offscreen opacity pipeline" ,vkCreateGraphicsPipelines, device, simplePipelineCache.pipelineCache, 1, &pipeline_CIO, nullptr, &offscreenPipeline);



    // 11-2 create opaque object pipelines
    VkPipelineShaderStageCreateInfo sceneStages[] = { sceneVertShaderStageCreateInfo, sceneFragShaderStageCreateInfo};
    rasterization_ST_CIO.cullMode = VK_CULL_MODE_NONE; // support foliage
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    pipeline_CIO.layout = scenePipelineLayout;
    pipeline_CIO.stageCount = 2;
    pipeline_CIO.pStages = sceneStages;
    pipeline_CIO.renderPass = simplePass.pass;
    pipeline_CIO.pColorBlendState = &blend_ST_CIO;
    UT_Fn::invoke_and_check( "error create scene opacity pipeline" ,vkCreateGraphicsPipelines, device, simplePipelineCache.pipelineCache,
     1, &pipeline_CIO, nullptr, &scenePipeline.opacity);
    // opaque pipeline
    rasterization_ST_CIO.cullMode = VK_CULL_MODE_BACK_BIT;
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    UT_Fn::invoke_and_check( "error create scene opaque pipeline" ,vkCreateGraphicsPipelines, device, simplePipelineCache.pipelineCache,
        1, &pipeline_CIO, nullptr, &scenePipeline.opaque);

    UT_Fn::cleanup_shader_module(device, offscreenFragMoudule, offscreenVertMoudule);
    UT_Fn::cleanup_shader_module(device, sceneFragModule, sceneVertMoudule);
}

void Shadowmap_v2::recordCommandBuffer() {
    //vkResetCommandBuffer(activatedFrameCommandBufferToSubmit,/*VkCommandBufferResetFlagBits*/ 0); //0: command buffer reset
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin shadow command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    {

        std::vector<VkClearValue> shadowClearValues(1);
        shadowClearValues[0].depthStencil = { 1.0f, 0 };
        // --------------- generate shadow map process
        const auto shadowSwapchainExtent = VkExtent2D{shadowFramebuffer.width, shadowFramebuffer.height};

    	/*
    	auto shadowRenderpassBeginInfo = FnCommand::renderPassBeginInfo(shadowFramebuffer.framebuffer,
        	shadowFramebuffer.renderPass,
        	shadowSwapchainExtent,
        	shadowClearValues);
    	shadowRenderpassBeginInfo.renderPass = shadowFramebuffer.renderPass;*/
    	VkClearValue cleaValue;
    	cleaValue.depthStencil = { 1.0f, 0 };
    	VkRenderPassBeginInfo renderPassBeginInfo {};
    	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    	renderPassBeginInfo.renderPass = shadowFramebuffer.renderPass;
    	renderPassBeginInfo.framebuffer = shadowFramebuffer.framebuffer;
    	renderPassBeginInfo.renderArea.extent.width = 2048;
    	renderPassBeginInfo.renderArea.extent.height = 2048;
    	renderPassBeginInfo.clearValueCount = 1;
    	renderPassBeginInfo.pClearValues = &cleaValue;



        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , offscreenPipeline); // FIRST generate depth for opaque object
        VkDeviceSize offsets[1] = { 0 };
        auto viewport = FnCommand::viewport(2048,2048 );
        auto scissor = FnCommand::scissor(2048,2048);
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf,0, 1, &scissor);
        vkCmdPushConstants(cmdBuf,
                           offscreenPipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(float),
                           &disable_opacity_texture);

        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &gridGeo.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,gridGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipelineLayout, 0, 1, &offscreenSets.opaque, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, gridGeo.parts[0].indices.size(), 1, 0, 0, 0);

        vkCmdPushConstants(cmdBuf,
                           offscreenPipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(float),
                           &enable_opacity_texture);
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &foliageGeo.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,foliageGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipelineLayout, 0, 1, &offscreenSets.opacity, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, foliageGeo.parts[0].indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdBuf);
    }
    {
        // ----------  render scene, using shadow map
        std::vector<VkClearValue> sceneClearValues(2);
        sceneClearValues[0].color = {0.4, 0.4, 0.4, 1};
        sceneClearValues[1].depthStencil = { 1.0f, 0 };
        const auto sceneRenderPass = simplePass.pass;
        const auto sceneRenderExtent = simpleSwapchain.swapChainExtent;
        const auto sceneRenderPassBeginInfo = FnCommand::renderPassBeginInfo(activatedSwapChainFramebuffer, sceneRenderPass, sceneRenderExtent,sceneClearValues);
        vkCmdBeginRenderPass(cmdBuf, &sceneRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , scenePipeline.opaque); // FIRST generate depth for opaque object
        auto [vs_width , vs_height] = sceneRenderExtent;
        auto viewport = FnCommand::viewport(vs_width,vs_height );
        auto scissor = FnCommand::scissor(vs_width, vs_height);
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf,0, 1, &scissor);
        VkDeviceSize offsets[1] = { 0 };
        vkCmdPushConstants(cmdBuf,scenePipelineLayout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(float),&disable_opacity_texture);
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &gridGeo.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,gridGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipelineLayout, 0, 1, &sceneSets.opaque, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, gridGeo.parts[0].indices.size(), 1, 0, 0, 0);

        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , scenePipeline.opacity); // FIRST generate depth for opaque object
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf,0, 1, &scissor);
        vkCmdPushConstants(cmdBuf,scenePipelineLayout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(float),&enable_opacity_texture);
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &foliageGeo.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,foliageGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipelineLayout, 0, 1, &sceneSets.opacity, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, foliageGeo.parts[0].indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdBuf);
    }
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}





LLVK_NAMESPACE_END
