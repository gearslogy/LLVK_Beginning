//
// Created by liuya on 9/1/2024.
//

#include "shadowmap.h"
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
#include "Pipeline.hpp"
LLVK_NAMESPACE_BEGIN
void shadowmap::render() {

}

void shadowmap::cleanupObjects() {
    auto device = mainDevice.logicalDevice;
    vkDestroyDescriptorPool(device, descPool, nullptr);
    UT_Fn::cleanup_resources(geoBufferManager, foliageTex, gridTex);
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_pipeline(device, offscreenPipelines.opaque, offscreenPipelines.opacity);
    UT_Fn::cleanup_pipeline(device, scenePipeline.opacity, scenePipeline.opaque);
    UT_Fn::cleanup_descriptor_set_layout(device, offscreenDescriptorSetLayout);
    UT_Fn::cleanup_pipeline_layout(device, offscreenPipelineLayout);
    cleanupOffscreenFramebuffer();
}

void shadowmap::prepare() {
    colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    shadowFramebuffer.depthSampler = FnImage::createDepthSampler(mainDevice.logicalDevice);
    loadTextures();
    loadModels();
    createOffscreenRenderPass();
    createOffscreenFramebuffer();
    prepareUniformBuffers();
    prepareDescriptorSets();
    preparePipelines();
}

void shadowmap::loadTextures() {
    setRequiredObjects(foliageTex, gridTex);
    foliageTex.create("content/plants/gardenplants/tex_array.ktx2", colorSampler );
    gridTex.create("content/shadowmap/grid_tex/tex_array.ktx2",colorSampler);
}

void shadowmap::loadModels() {
    setRequiredObjects(geoBufferManager);
    gridGeo.load("content/shadowmap/grid.gltf");
    foliageGeo.load("content/plants/gardenplants/var0.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(gridGeo, geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(foliageGeo, geoBufferManager);
}

void shadowmap::createOffscreenRenderPass() {
    VkAttachmentDescription depthAttachmentDescription = {};
    depthAttachmentDescription.format = shadowFramebuffer.depthAttachment.format;
    depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store the shadow map!
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                        // beginning layout
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;    // finally to this layout!

    VkAttachmentReference depthReference{};
    depthReference.attachment = 0;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &depthAttachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    UT_Fn::invoke_and_check("Error created render pass",vkCreateRenderPass, mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &shadowFramebuffer.renderPass);
}

void shadowmap::createOffscreenFramebuffer() {
    const auto device = mainDevice.logicalDevice;
    setRequiredObjects(shadowFramebuffer.depthAttachment);
    shadowFramebuffer.depthAttachment.create(shadowFramebuffer.width,
        shadowFramebuffer.height,
        FnImage::findDepthFormat(mainDevice.physicalDevice), shadowFramebuffer.depthSampler,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT // create function will auto add VK_IMAGE_USAGE_SAMPLED_BIT
        );
    // framebuffer create
    std::array attachments = {shadowFramebuffer.depthAttachment.view};
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = shadowFramebuffer.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = shadowFramebuffer.width;           // FIXED shadow map size;
    framebufferInfo.height = shadowFramebuffer.height;         // FIXED shadow map size;
    framebufferInfo.layers = 1;
    framebufferInfo.pAttachments = attachments.data();
    UT_Fn::invoke_and_check("create framebuffer failed", vkCreateFramebuffer,device, &framebufferInfo, nullptr, &shadowFramebuffer.framebuffer);
}

void shadowmap::cleanupOffscreenFramebuffer() {
    const auto device = mainDevice.logicalDevice;
    vkDestroyFramebuffer(device, shadowFramebuffer.framebuffer, nullptr);
    vkDestroyRenderPass(device, shadowFramebuffer.renderPass, nullptr);
    shadowFramebuffer.depthAttachment.cleanup();
    UT_Fn::cleanup_render_pass(device, shadowFramebuffer.renderPass);
    UT_Fn::cleanup_sampler(device, shadowFramebuffer.depthSampler);
}




void shadowmap::prepareUniformBuffers() {
    setRequiredObjects(uniformBuffers.scene, uniformBuffers.scene);
    uniformBuffers.offscreen.createAndMapping(sizeof(uniformDataOffscreen));
    uniformBuffers.scene.createAndMapping(sizeof(uniformDataScene));
    lightPos = {281.654, 274.945,316.942};
    updateUniformBuffers();

}
void shadowmap::updateUniformBuffers() {
    // offscreen
    uniformDataScene.zNear = 0.1;
    uniformDataScene.zFar = 96.0;
    const auto depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, uniformDataScene.zNear, uniformDataScene.zFar);
    const auto depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    const auto depthModelMatrix = glm::mat4(1.0f);
    uniformDataOffscreen.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
    memcpy(uniformBuffers.offscreen.mapped, &uniformDataOffscreen, sizeof(uniformDataOffscreen));

    uniformDataScene.projection = mainCamera.projection();
    uniformDataScene.view = mainCamera.view();
    uniformDataScene.model = glm::mat4(1.0f);
    uniformDataScene.depthBiasMVP = uniformDataOffscreen.depthMVP;
    memcpy(uniformBuffers.scene.mapped, &uniformDataScene, sizeof(uniformDataScene));
}

void shadowmap::prepareDescriptorSets() {
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
    auto sceneAllocInfo = FnDescriptor::setAllocateInfo(descPool,scene_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create scene sets",vkAllocateDescriptorSets,device, &sceneAllocInfo,&sceneSets.opacity );
    UT_Fn::invoke_and_check("Error create scene sets",vkAllocateDescriptorSets,device, &sceneAllocInfo,&sceneSets.opaque );
    // write set
    // - offscreen opacity
    std::vector<VkWriteDescriptorSet> opacityWriteSets;
    opacityWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(offscreenSets.opacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.offscreen.descBufferInfo));
    opacityWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(offscreenSets.opacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1, &foliageTex.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opacityWriteSets.size()),opacityWriteSets.data(),0, nullptr);
    // - offscreen opaque
    std::vector<VkWriteDescriptorSet> opaqueWriteSets;
    opaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(offscreenSets.opaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.offscreen.descBufferInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opaqueWriteSets.size()),opaqueWriteSets.data(),0, nullptr);

    // - scene opacity
    std::vector<VkWriteDescriptorSet> opacitySceneWriteSets;
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.scene.descBufferInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1, &foliageTex.descImageInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2, &shadowFramebuffer.depthAttachment.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opacitySceneWriteSets.size()),opacitySceneWriteSets.data(),0, nullptr);

    // - scene opaque
    std::vector<VkWriteDescriptorSet> sceneOpaqueWriteSets;
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.scene.descBufferInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1, &gridTex.descImageInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2, &shadowFramebuffer.depthAttachment.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(sceneOpaqueWriteSets.size()),sceneOpaqueWriteSets.data(),0, nullptr);
}



void shadowmap::preparePipelines() {
    auto device = mainDevice.logicalDevice;
    const auto offscreenVertMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/offscreen_vert.spv",  device);
    const auto offscreenFragMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/offscreen.spv",  device);

    const auto sceneVertMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/scene_vert.spv",  device);
    const auto sceneOpacityFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/scene_frag.spv",  device); // need VK_CULLING_NONE

    VkPipelineShaderStageCreateInfo offscreenVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, offscreenVertMoudule);
    VkPipelineShaderStageCreateInfo offscreenFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, offscreenFragMoudule);
    VkPipelineShaderStageCreateInfo sceneVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, sceneVertMoudule);
    VkPipelineShaderStageCreateInfo sceneFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, sceneOpacityFragModule);

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
    constexpr VkPushConstantRange pushRanges[pushCount] = {{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(constantData) } };
    offscreenSetLayout_CIO.pPushConstantRanges = pushRanges;
    offscreenSetLayout_CIO.pushConstantRangeCount = pushCount;
    UT_Fn::invoke_and_check("ERROR create offscreen pipeline layout",vkCreatePipelineLayout,device, &offscreenSetLayout_CIO,nullptr, &offscreenPipelineLayout );
    // 9-2 scene pipeline layout
    const std::array sceneSetLayouts{sceneDescriptorSetLayout};
    VkPipelineLayoutCreateInfo sceneSetLayout_CIO = FnPipeline::layoutCreateInfo(sceneSetLayouts);
    UT_Fn::invoke_and_check("ERROR create scene pipeline layout",vkCreatePipelineLayout,device, &offscreenSetLayout_CIO,nullptr, &scenePipelineLayout );

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
    rasterization_ST_CIO.cullMode = VK_CULL_MODE_NONE; // support foliage
    VkPipelineShaderStageCreateInfo offscreenStages[] = { offscreenVertShaderStageCreateInfo, offscreenFragShaderStageCreateInfo};
    pipeline_CIO.renderPass = shadowFramebuffer.renderPass;
    pipeline_CIO.layout = offscreenPipelineLayout;
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    pipeline_CIO.pStages = offscreenStages;
    // opacity
    UT_Fn::invoke_and_check( "error create offscreen opacity pipeline" ,vkCreateGraphicsPipelines, device, simplePipelineCache.pipelineCache,
         1, &pipeline_CIO, nullptr, &offscreenPipelines.opacity);
    // opaque
    rasterization_ST_CIO.cullMode = VK_CULL_MODE_BACK_BIT;
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    UT_Fn::invoke_and_check( "error create offscreen opaque pipeline" ,vkCreateGraphicsPipelines, device, simplePipelineCache.pipelineCache,
           1, &pipeline_CIO, nullptr, &offscreenPipelines.opaque);


    // 11-2 create opaque object pipelines
    rasterization_ST_CIO.cullMode = VK_CULL_MODE_NONE; // support foliage
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    pipeline_CIO.layout = scenePipelineLayout;
    VkPipelineShaderStageCreateInfo sceneStages[] = { sceneVertShaderStageCreateInfo, sceneFragShaderStageCreateInfo};
    pipeline_CIO.pStages = sceneStages;
    UT_Fn::invoke_and_check( "error create scene opacity pipeline" ,vkCreateGraphicsPipelines, device, simplePipelineCache.pipelineCache,
     1, &pipeline_CIO, nullptr, &scenePipeline.opacity);



    rasterization_ST_CIO.cullMode = VK_CULL_MODE_BACK_BIT;
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;

}




LLVK_NAMESPACE_END
