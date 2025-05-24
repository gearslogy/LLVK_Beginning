#include "dynamicsUBO.h"
#include <vector>
#include "Pipeline.hpp"
#include <ranges>
#include <algorithm>
#include <random>
#include "LLVK_Math.hpp"
#include "CommandManager.h"
#include "GeoVertexDescriptions.h"
LLVK_NAMESPACE_BEGIN
void DynamicsUBO::cleanupObjects() {

    auto device = mainDevice.logicalDevice;
    UT_Fn::cleanup_range_resources(plantTextures);
    UT_Fn::cleanup_range_resources(groundTextures);
    vkDestroySampler(device, sampler, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    UT_Fn::cleanup_descriptor_set_layout(device,plantUBOSetLayout, plantTextureSetLayout);
    UT_Fn::cleanup_descriptor_set_layout(device,standardPipeline.uboSetLayout, standardPipeline.textureSetLayout);
    UT_Fn::cleanup_pipeline_layout(device,standardPipeline.pipelineLayout, plantPipelineLayout);
    UT_Fn::cleanup_pipeline(device, plantPipeline, standardPipeline.pipeline);
    geometryBufferManager.cleanup();
    // uniform buffer clean up
    standardUniformBuffer.cleanup();
    plantUniformBuffers.viewBuffer.cleanup();
    plantUniformBuffers.dynamicBuffer.cleanup();
    if (uboDataDynamic.model) {
        alignedFree(uboDataDynamic.model);
    }

}



void DynamicsUBO::loadPlantTextures() {
    std::cout << "[[load plant texture]]\n";
    const auto phyDevice = mainDevice.physicalDevice;
    const auto device = mainDevice.logicalDevice;
    std::vector<std::string> files{
        "content/plants/gardenplants/ForestFern_2K_Albedo.jpg",
        "content/plants/gardenplants/ForestFern_2K_Displacement.jpg",
        "content/plants/gardenplants/ForestFern_2K_Normal.jpg",
        "content/plants/gardenplants/ForestFern_2K_Opacity.jpg",
        "content/plants/gardenplants/ForestFern_2K_Roughness.jpg",
        "content/plants/gardenplants/ForestFern_2K_Translucency.jpg",
    };
    for(auto&& [file, tex] : std::views::zip(files, plantTextures)) {
        // first bind obj
        tex.requiredObjs.device = device;
        tex.requiredObjs.physicalDevice = phyDevice;
        tex.requiredObjs.commandPool = graphicsCommandPool;
        tex.requiredObjs.queue = mainDevice.graphicsQueue;
        tex.create(file,sampler);
    }

}

void DynamicsUBO::loadGroundTextures() {
    std::cout << "[[load ground texture]]\n";
    const auto phyDevice = mainDevice.physicalDevice;
    const auto device = mainDevice.logicalDevice;
    std::vector<std::string> files{
        "content/ground/xbreair_2K_Albedo.jpg",
        "content/ground/xbreair_2K_AO.jpg",
        "content/ground/xbreair_2K_Displacement.jpg",
        "content/ground/xbreair_2K_Normal.jpg",
        "content/ground/xbreair_2K_Roughness.jpg",
    };
    for(auto&& [file, tex] : std::views::zip(files, groundTextures)) {
        // first bind obj
        tex.requiredObjs.device = device;
        tex.requiredObjs.physicalDevice = phyDevice;
        tex.requiredObjs.commandPool = graphicsCommandPool;
        tex.requiredObjs.queue = mainDevice.graphicsQueue;
        tex.create(file,sampler);
    }

}

void DynamicsUBO::loadTexture() {
    const auto phyDevice = mainDevice.physicalDevice;
    const auto device = mainDevice.logicalDevice;
    sampler = FnImage::createImageSampler(phyDevice, device); // create sampler. shared sampler for all image
    loadPlantTextures();
    loadGroundTextures();
}
void DynamicsUBO::loadModel() {
    plantGeo.load("content/plants/gardenplants/var0.gltf");
    createVertexAndIndexBuffer<GLTFVertex>(geometryBufferManager,  plantGeo.parts[0]);
    groundGeo.load("content/ground/ground.gltf");
    createVertexAndIndexBuffer<GLTFVertex>(geometryBufferManager,  groundGeo.parts[0]);
}

void DynamicsUBO::setupDescriptors() {
    namespace FnDesc = FnDescriptor;
    auto device = mainDevice.logicalDevice;

    std::array<VkDescriptorPoolSize, 3> poolSizes= {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},         // VP Matrix
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 }, // M Matrix
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, plantNumPlantsImages * 2}   // plant:6 Combined Image Sampler, ground:5 combined image sampler
    }};
    // 2个set(set=0服务UBO，set=1服务Texture)
    // set0 set1 : plant
    // set2 set3 : ground
    // so we need four set
    VkDescriptorPoolCreateInfo createInfo = FnDesc::poolCreateInfo(poolSizes, 2*2); //

    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    if(vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool)!=VK_SUCCESS)
        throw std::runtime_error{"ERROR create descriptor pool"};

    // create UBO set layout
    const auto set0_binding0 = FnDesc::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);
    const auto set0_binding1 = FnDesc::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT);
    const std::array set0_bindings = {set0_binding0, set0_binding1};
    const VkDescriptorSetLayoutCreateInfo ubo_createInfo = FnDesc::setLayoutCreateInfo(set0_bindings);
    if(vkCreateDescriptorSetLayout(device, &ubo_createInfo, nullptr, &plantUBOSetLayout)!=VK_SUCCESS)
        throw std::runtime_error{"Error create plant ubo set layout"};

    // create texture set layout, albedo/AO/disp/N/Roughness/Opacity
    constexpr auto combinedImageLayoutBinding = [](const int &binding){return FnDesc::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,binding, VK_SHADER_STAGE_FRAGMENT_BIT);};
    auto bindings = std::views::iota(0,plantNumPlantsImages) | std::views::transform(combinedImageLayoutBinding);
    const auto set1_textureBindings=  std::ranges::to<std::vector<VkDescriptorSetLayoutBinding>>(bindings);
    const VkDescriptorSetLayoutCreateInfo tex_createInfo = FnDesc::setLayoutCreateInfo(set1_textureBindings);
    if(vkCreateDescriptorSetLayout(device, &tex_createInfo, nullptr, &plantTextureSetLayout)!=VK_SUCCESS)
        throw std::runtime_error{"Error create plant tex set layout"};

    // create plant sets based on setLayouts
    const std::array plantLayouts{plantUBOSetLayout, plantTextureSetLayout};
    const auto plantSetAllocateInfo = FnDesc::setAllocateInfo(descriptorPool,plantLayouts);
    if(vkAllocateDescriptorSets(device, &plantSetAllocateInfo,plantDescriptorSets) != VK_SUCCESS)
        throw std::runtime_error{"can not create plant descriptor set"};

    // dynamics descritpor buffer info should modify
    plantUniformBuffers.dynamicBuffer.descBufferInfo.range = dynamicAlignment; // ! important !

    // -- write set end--
    std::vector plantWriteSets{
        FnDesc::writeDescriptorSet(plantDescriptorSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &plantUniformBuffers.viewBuffer.descBufferInfo ), // set=0 binding=0
        FnDesc::writeDescriptorSet(plantDescriptorSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, &plantUniformBuffers.dynamicBuffer.descBufferInfo ),// set=0 binding=1
    };
    for(auto &&[k,v] : UT_Fn::enumerate(plantTextures) ) {
        auto writeSet = FnDesc::writeDescriptorSet(plantDescriptorSets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k, &v.descImageInfo );
        plantWriteSets.emplace_back(writeSet);
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(plantWriteSets.size()), plantWriteSets.data(), 0, nullptr);



    // ------------------ next ground descriptor set. use same layout that is plant layout-----------------------
    const std::array standard_set0_bindings = {set0_binding0};
    const VkDescriptorSetLayoutCreateInfo standard_ubo_createInfo = FnDesc::setLayoutCreateInfo(standard_set0_bindings);
    if(vkCreateDescriptorSetLayout(device, &standard_ubo_createInfo, nullptr, &standardPipeline.uboSetLayout)!=VK_SUCCESS) // set = 0
        throw std::runtime_error{"Error create standard ubo set layout"};
    if(vkCreateDescriptorSetLayout(device, &tex_createInfo, nullptr, &standardPipeline.textureSetLayout)!=VK_SUCCESS)      // set = 1
        throw std::runtime_error{"Error create standard tex set layout"};
    //create pipeline layout of standard
    const std::array standardLayouts{standardPipeline.uboSetLayout, standardPipeline.textureSetLayout};
    const auto standardSetAllocateInfo = FnDesc::setAllocateInfo(descriptorPool,standardLayouts);
    if(vkAllocateDescriptorSets(device, &standardSetAllocateInfo,standardPipeline.sets) != VK_SUCCESS)
        throw std::runtime_error{"can not create standard descriptor set"};
    std::vector standardWriteSets{
        FnDesc::writeDescriptorSet(standardPipeline.sets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &standardUniformBuffer.descBufferInfo ), // set=0 binding=0 ubo
    };
    for(auto &&[k,v] : UT_Fn::enumerate(groundTextures) ) {
        auto writeSet = FnDesc::writeDescriptorSet(standardPipeline.sets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k, &v.descImageInfo );       // set=1 binding=0-5 textures
        standardWriteSets.emplace_back(writeSet);
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(standardWriteSets.size()), standardWriteSets.data(), 0, nullptr);
}
void DynamicsUBO::preparePipelines() {
    auto device = mainDevice.logicalDevice;
    const auto plantVertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/dynamicsUBO_vert.spv",  device);
    const auto plantFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/dynamicsUBO_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, plantVertModule);
    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, plantFragModule);
    // 1
    VkPipelineShaderStageCreateInfo shaderStates[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};
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
    std::array colorBlendAttamentState = {FnPipeline::simpleOpaqueColorBlendAttachmentState()};
    VkPipelineColorBlendStateCreateInfo blend_ST_CIO = FnPipeline::colorBlendStateCreateInfo(colorBlendAttamentState);

    // 9. pipeline layout
    const std::array plantLayouts{plantUBOSetLayout, plantTextureSetLayout};
    VkPipelineLayoutCreateInfo layout_CIO = FnPipeline::layoutCreateInfo(plantLayouts);
    // create pipeline layout
    auto result = vkCreatePipelineLayout(device, &layout_CIO, nullptr, &plantPipelineLayout);
    if(result != VK_SUCCESS) throw std::runtime_error{"ERROR create pipeline layout"};

    // 10
    VkPipelineDepthStencilStateCreateInfo ds_ST_CIO = FnPipeline::depthStencilStateCreateInfoEnabled();
    // 11. PIPELINE
    VkGraphicsPipelineCreateInfo pipeline_CIO = FnPipeline::pipelineCreateInfo();
    pipeline_CIO.stageCount = 2;
    pipeline_CIO.pStages = shaderStates;
    pipeline_CIO.pVertexInputState = &vertexInput_ST_CIO;
    pipeline_CIO.pInputAssemblyState = &inputAssembly_ST_CIO;
    pipeline_CIO.pViewportState = &viewport_ST_CIO;
    pipeline_CIO.pDynamicState = &dynamics_ST_CIO;
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    pipeline_CIO.pMultisampleState = &multisample_ST_CIO;
    pipeline_CIO.pColorBlendState = &blend_ST_CIO;
    pipeline_CIO.pDepthStencilState = &ds_ST_CIO;
    pipeline_CIO.layout = plantPipelineLayout;
    pipeline_CIO.renderPass = simplePass.pass ;
    pipeline_CIO.subpass = 0; // ONLY USE ONE PASS
    result = vkCreateGraphicsPipelines(device, simplePipelineCache.pipelineCache,
        1, &pipeline_CIO, nullptr, &plantPipeline);
    if(result!= VK_SUCCESS) throw std::runtime_error{"Failed created graphics pipeline"};
    // finally destory shader module
    UT_Fn::cleanup_shader_module(device, plantVertModule, plantFragModule);

    // create standard pipeline.
    const auto standardVertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/standard_vert.spv",  device);
    const auto standardfragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/standard_frag.spv",  device);
    VkPipelineShaderStageCreateInfo standardVert_SCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, standardVertModule);
    VkPipelineShaderStageCreateInfo standardFrag_SCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, standardfragModule);
    VkPipelineShaderStageCreateInfo standardShaderStates[] = {standardVert_SCIO, standardFrag_SCIO};
    //  2. vertex same as plant
    //  3. assembly
    //  4. viewport and scissor
    //  5. dynamic state
    //  6. rasterization
    VkPipelineRasterizationStateCreateInfo standardRasterization_ST_CIO = FnPipeline::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    //  7. multisampling
    //  8. blending
    //  9. pipeline Layout
    // create pipeline layout
    const std::array standardLayouts{standardPipeline.uboSetLayout, standardPipeline.textureSetLayout};
    VkPipelineLayoutCreateInfo standardLayout_CIO = FnPipeline::layoutCreateInfo(standardLayouts);
    result = vkCreatePipelineLayout(device, &standardLayout_CIO, nullptr, &standardPipeline.pipelineLayout);
    if(result != VK_SUCCESS) throw std::runtime_error{"ERROR create pipeline layout"};

    // 10. DS same
    // 11. PIPELINE
    pipeline_CIO.stageCount = 2;
    pipeline_CIO.pStages = standardShaderStates;
    pipeline_CIO.pRasterizationState = &standardRasterization_ST_CIO;
    pipeline_CIO.layout = standardPipeline.pipelineLayout;
    result = vkCreateGraphicsPipelines(device, simplePipelineCache.pipelineCache,
        1, &pipeline_CIO, nullptr, &standardPipeline.pipeline);
    if(result!= VK_SUCCESS) throw std::runtime_error{"Failed created graphics pipeline"};
    UT_Fn::cleanup_shader_module(device, standardVertModule, standardfragModule);

}

void DynamicsUBO::prepareUniformBuffers() {
    auto device = mainDevice.logicalDevice;
    auto phyDevice = mainDevice.physicalDevice;

    VkPhysicalDeviceProperties gpuProps{};
    vkGetPhysicalDeviceProperties(phyDevice, &gpuProps);
    const size_t minUBOAlignment = gpuProps.limits.minUniformBufferOffsetAlignment; // RTX 3070 64

    dynamicAlignment = sizeof(glm::mat4);
    if (minUBOAlignment > 0) {
        dynamicAlignment = (dynamicAlignment + minUBOAlignment - 1) & ~(minUBOAlignment - 1);
    }


    std::cout << "minUniformBufferOffsetAlignment = " << minUBOAlignment << std::endl;      // 64
    std::cout << "dynamicAlignment = " << dynamicAlignment << std::endl;                    // 64

    size_t bufferSize = OBJECT_INSTANCES * dynamicAlignment;
    uboDataDynamic.model = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment); // 动态内存对齐开辟
    assert(uboDataDynamic.model);
    // Static shared uniform buffer object with projection and view matrix
    plantUniformBuffers.viewBuffer.create(sizeof(uboVS));
    // Uniform buffer object with per-object matrices
    std::cout << "created dynamicBuffer size = " << bufferSize << std::endl;                    // 64
    plantUniformBuffers.dynamicBuffer.create(bufferSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    // Map persistent
    plantUniformBuffers.viewBuffer.map();
    plantUniformBuffers.dynamicBuffer.map();
    float radius = 150;
    for (const auto &idx : std::views::iota(0, OBJECT_INSTANCES)) {
        positions[idx] = glm::vec3{random_double(-1,1)*radius, 0, random_double(-1,1)*radius    };
        yRotations[idx] = random_double(0,1)*360;
        scales[idx] = random_double(0,1);
    }

    // ground uniform buffer
    standardUniformBuffer.create(sizeof(uboStandardData));
    standardUniformBuffer.map();

    updateUniformBuffers();
    updateDynamicUniformBuffer();
}

void DynamicsUBO::updateUniformBuffers() {
    // Fixed ubo with projection and view matrices
    auto [width, height] = simpleSwapchain.swapChainExtent;
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboVS.projection = mainCamera.projection();
    uboVS.projection[1][1] *= -1;
    uboVS.view = mainCamera.view();
    memcpy(plantUniformBuffers.viewBuffer.mapped, &uboVS, sizeof(uboVS));

    uboStandardData.model = 1.0f;
    uboStandardData.proj = uboVS.projection;
    uboStandardData.view = uboVS.view;
    memcpy(standardUniformBuffer.mapped, &uboStandardData, sizeof(uboStandardData));
}

void DynamicsUBO::updateDynamicUniformBuffer() {
    // Dynamic ubo with per-object model matrices indexed by offsets in the command buffer
    for (const auto &idx : std::views::iota(0, OBJECT_INSTANCES)) {
        auto axis = glm::vec3{0,1,0};
        auto R = glm::rotate(glm::mat4{1.0f},yRotations[idx],axis);
        auto S = glm::scale(glm::mat4{1.0f}, glm::vec3{scales[idx]});
        auto T = glm::translate(glm::mat4{1.0f}, positions[idx]);
        uboDataDynamic.model[idx] =     T * R * S;

    }

    memcpy(plantUniformBuffers.dynamicBuffer.mapped, uboDataDynamic.model, plantUniformBuffers.dynamicBuffer.descBufferInfo.range);
    // Flush to make changes visible to the host
    VkMappedMemoryRange mappedMemoryRange {};
    mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedMemoryRange.memory =plantUniformBuffers.dynamicBuffer.memory;
    mappedMemoryRange.size = plantUniformBuffers.dynamicBuffer.memorySize;
    vkFlushMappedMemoryRanges(mainDevice.logicalDevice, 1, &mappedMemoryRange);
}




void DynamicsUBO::bindResources() {
    auto device = mainDevice.logicalDevice;
    auto phyDevice = mainDevice.physicalDevice;
    geometryBufferManager.bindDevice = mainDevice.logicalDevice;
    geometryBufferManager.bindQueue = mainDevice.graphicsQueue;
    geometryBufferManager.bindCommandPool = graphicsCommandPool;
    geometryBufferManager.bindPhysicalDevice = mainDevice.physicalDevice;

    plantUniformBuffers.viewBuffer.requiredObjs.device = device;
    plantUniformBuffers.viewBuffer.requiredObjs.physicalDevice = phyDevice;
    plantUniformBuffers.dynamicBuffer.requiredObjs.device = device;
    plantUniformBuffers.dynamicBuffer.requiredObjs.physicalDevice = phyDevice;

    standardUniformBuffer.requiredObjs.device = device;
    standardUniformBuffer.requiredObjs.physicalDevice = phyDevice;
    standardUniformBuffer.requiredObjs.device = device;
    standardUniformBuffer.requiredObjs.physicalDevice = phyDevice;

}

void DynamicsUBO::recordCommandBuffer() {
  // select framebuffer
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    const VkFramebuffer &framebuffer = getMainFramebuffer();
    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(framebuffer,
        simplePass.pass,
        &simpleSwapchain.swapChainExtent,clearValues);

    auto cmdBuf = getMainCommandBuffer();
    auto result = vkBeginCommandBuffer(cmdBuf, &cmdBufferBeginInfo);
    if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
    vkCmdBeginRenderPass(cmdBuf, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS ,plantPipeline);

    auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);


    VkDeviceSize offsets[1] = { 0 };

    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &plantGeo.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(cmdBuf,plantGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);


    // Render multiple objects using different model matrices by dynamically offsetting into one uniform buffer
    for (uint32_t j = 0; j < OBJECT_INSTANCES; j++)
    {
        // One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
        uint32_t dynamicOffset = j * static_cast<uint32_t>(dynamicAlignment);
        // Bind the descriptor set for rendering a mesh using the dynamic offset
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, plantPipelineLayout, 0, 2, plantDescriptorSets, 1, &dynamicOffset);
        vkCmdDrawIndexed(cmdBuf, plantGeo.parts[0].indices.size(), 1, 0, 0, 0);
    }

    // render ground
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS ,standardPipeline.pipeline);
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &groundGeo.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(cmdBuf,groundGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, standardPipeline.pipelineLayout, 0, 2, standardPipeline.sets, 0, nullptr);
    vkCmdDrawIndexed(cmdBuf, groundGeo.parts[0].indices.size(), 1, 0, 0, 0);


    vkCmdEndRenderPass(cmdBuf);
    if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

LLVK_NAMESPACE_END
