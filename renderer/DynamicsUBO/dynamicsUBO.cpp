#include "dynamicsUBO.h"
#include <vector>
#include "Pipeline.hpp"
#include <ranges>
#include <algorithm>
#include <random>
#include "LLVK_Math.hpp"
#include "CommandManager.h"
LLVK_NAMESPACE_BEGIN
void DynamicsUBO::cleanupObjects() {

    auto device = mainDevice.logicalDevice;
    UT_Fn::cleanup_range_resources(plantTextures);
    vkDestroySampler(device, sampler, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyPipelineLayout(device,plantPipelineLayout,nullptr);
    vkDestroyDescriptorSetLayout(device, plantUBOSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, plantTextureSetLayout, nullptr);
    vkDestroyPipeline(device,plantPipeline,nullptr);
    geometryBufferManager.cleanup();
    // uniform buffer clean up
    plantUniformBuffers.viewBuffer.cleanup();
    plantUniformBuffers.dynamicBuffer.cleanup();
    if (uboDataDynamic.model) {
        alignedFree(uboDataDynamic.model);
    }

}



void DynamicsUBO::loadPlantTextures() {
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

    const auto phyDevice = mainDevice.physicalDevice;
    const auto device = mainDevice.logicalDevice;
    std::vector<std::string> files{
        "content/ground/xbreair_2K_Albedo.jpg",
        "content/ground/xbreair_2K_AO.jpg",
        "content/ground/xbreair_2K_Displacement.jpg",
        "content/ground/xbreair_2K_Normal.jpg",
        "content/ground/xbreair_2K_Roughness.jpg",
    };


}

void DynamicsUBO::loadTexture() {
    const auto phyDevice = mainDevice.physicalDevice;
    const auto device = mainDevice.logicalDevice;
    sampler = FnImage::createImageSampler(phyDevice, device); // create sampler. shared sampler for all image
    loadPlantTextures();
    //loadGroundTextures();
}
void DynamicsUBO::loadModel() {
    plantGeo.readFile("content/plants/gardenplants/var0.obj");
    createVertexAndIndexBuffer(geometryBufferManager, plantGeo);
    //groundGeo.readFile("content/ground/ground.obj");
    //createVertexAndIndexBuffer(geometryBufferManager, groundGeo);
}

void DynamicsUBO::setupDescriptors() {
    namespace FnDesc = LLVK::FnDescriptor;
    auto device = mainDevice.logicalDevice;

    std::array<VkDescriptorPoolSize, 3> poolSizes= {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},         // VP Matrix
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 }, // M Matrix
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, plantNumPlantsImages}   // 6个Combined Image Sampler
    }};

    VkDescriptorPoolCreateInfo createInfo = FnDesc::poolCreateInfo(poolSizes, 2); // 2个set(set=0服务UBO，set=1服务Texture)
    if(vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool)!=VK_SUCCESS)
        throw std::runtime_error{"ERROR create descriptor pool"};

    // create UBO set layout
    const auto set0_binding0 = FnDesc::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);
    const auto set0_binding1 = FnDesc::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT);
    const std::array set0_bindings = {set0_binding0, set0_binding1};
    const VkDescriptorSetLayoutCreateInfo ubo_createInfo = FnDesc::setLayoutCreateInfo(set0_bindings);
    if(vkCreateDescriptorSetLayout(device, &ubo_createInfo, nullptr, &plantUBOSetLayout)!=VK_SUCCESS)
        throw std::runtime_error{"Error create set layout"};

    // create texture set layout, albedo/AO/disp/N/Roughness/Opacity
    constexpr auto combinedImageLayoutBinding = [](const int &binding){return FnDesc::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,binding, VK_SHADER_STAGE_FRAGMENT_BIT);};
    auto bindings = std::views::iota(0,plantNumPlantsImages) | std::views::transform(combinedImageLayoutBinding);
    const auto set1_textureBindings=  std::ranges::to<std::vector<VkDescriptorSetLayoutBinding>>(bindings);
    const VkDescriptorSetLayoutCreateInfo tex_createInfo = FnDesc::setLayoutCreateInfo(set1_textureBindings);
    if(vkCreateDescriptorSetLayout(device, &tex_createInfo, nullptr, &plantTextureSetLayout)!=VK_SUCCESS)
        throw std::runtime_error{"error create set layout"};

    // create plant sets based on setLayouts
    const std::array plantLayouts{plantUBOSetLayout, plantTextureSetLayout};
    const auto plantSetAllocateInfo = FnDesc::setAllocateInfo(descriptorPool,plantLayouts);
    if(vkAllocateDescriptorSets(device, &plantSetAllocateInfo,plantDescriptorSets) != VK_SUCCESS)
        throw std::runtime_error{"can not create descriptor set"};

    // dynamics descritpor buffer info should modify
    plantUniformBuffers.dynamicBuffer.descBufferInfo.range = dynamicAlignment; // ! important !

    // -- write set end--
    std::vector writeSets{
        FnDesc::writeDescriptorSet(plantDescriptorSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &plantUniformBuffers.viewBuffer.descBufferInfo ), // set=0 binding=0
        FnDesc::writeDescriptorSet(plantDescriptorSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, &plantUniformBuffers.dynamicBuffer.descBufferInfo ),// set=0 binding=1
    };
    for(auto &&[k,v] : std::views::zip(UT_Fn::xrange(0,plantTextures),plantTextures  ) ) {
        auto writeSet = FnDesc::writeDescriptorSet(plantDescriptorSets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k, &v.descImageInfo );
        writeSets.emplace_back(writeSet);
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

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
    std::array bindings = {Vertex::bindings()};
    auto attribs = Vertex::attribs();
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

    // 9. pipeline layout
    const std::array plantLayouts{plantUBOSetLayout, plantTextureSetLayout};
    VkPipelineLayoutCreateInfo layout_CIO = FnPipeline::layoutCreateInfo(plantLayouts);
    // create pipeline layout
    auto result = vkCreatePipelineLayout(device, &layout_CIO, nullptr, &plantPipelineLayout);
    if(result != VK_SUCCESS) throw std::runtime_error{"ERROR create pipeline layout"};

    // 10
    VkPipelineDepthStencilStateCreateInfo ds_ST_CIO = FnPipeline::depthStencilStateCreateInfoEnabled();
    // 11. PIPELINE
    VkGraphicsPipelineCreateInfo pipeline_CIO{};
    pipeline_CIO.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
    // can create multi pipeline that derive from one atnoher for optimisation
    pipeline_CIO.basePipelineHandle = VK_NULL_HANDLE; // exsting pipeline to derive from.
    pipeline_CIO.basePipelineIndex = -1;              // or index of pipeline being created to derive from
    result = vkCreateGraphicsPipelines(device, simplePipelineCache.pipelineCache,
        1, &pipeline_CIO, nullptr, &plantPipeline);
    if(result!= VK_SUCCESS) throw std::runtime_error{"Failed created graphics pipeline"};
    // finally destory shader module
    vkDestroyShaderModule(device, plantVertModule, nullptr);
    vkDestroyShaderModule(device, plantFragModule, nullptr);



}

void DynamicsUBO::prepareUniformBuffers() {
    auto device = mainDevice.logicalDevice;
    auto phyDevice = mainDevice.physicalDevice;
    plantUniformBuffers.viewBuffer.requiredObjs.device = device;
    plantUniformBuffers.viewBuffer.requiredObjs.physicalDevice = phyDevice;
    plantUniformBuffers.dynamicBuffer.requiredObjs.device = device;
    plantUniformBuffers.dynamicBuffer.requiredObjs.physicalDevice = phyDevice;

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

    updateUniformBuffers();
    updateDynamicUniformBuffer();
}

void DynamicsUBO::updateUniformBuffers() {
    // Fixed ubo with projection and view matrices
    auto [width, height] = simpleSwapchain.swapChainExtent;
    uboVS.projection =  glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 2000.0f);
    uboVS.projection[1][1] *= -1;
    uboVS.view =  glm::lookAt(glm::vec3(0, 80.0f, 200), glm::vec3(0.0f, 0, 40.0f), glm::vec3(0.0f, 1.0f, 0.0f));// OGL
    memcpy(plantUniformBuffers.viewBuffer.mapped, &uboVS, sizeof(uboVS));
}

void DynamicsUBO::updateDynamicUniformBuffer() {
    // Dynamic ubo with per-object model matrices indexed by offsets in the command buffer
    float radius = 180;
    for (const auto &idx : std::views::iota(0, OBJECT_INSTANCES)) {

        auto pos = glm::vec3{random_double(-1,1)*radius, 0, random_double(-1,1)*radius    };
        auto axis = glm::vec3{0,1,0};
        float angle = random_double(0,1)*360;
        auto rot = glm::rotate(glm::mat4{1.0f},angle,axis);
        uboDataDynamic.model[idx] = glm::translate(rot, pos);
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
    geometryBufferManager.bindDevice = mainDevice.logicalDevice;
    geometryBufferManager.bindQueue = mainDevice.graphicsQueue;
    geometryBufferManager.bindCommandPool = graphicsCommandPool;
    geometryBufferManager.bindPhysicalDevice = mainDevice.physicalDevice;
}

void DynamicsUBO::recordCommandBuffer() {
  // select framebuffer
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    const VkFramebuffer &framebuffer = activeSwapChainFramebuffer;
    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(framebuffer,
        simplePass.pass,
        &simpleSwapchain.swapChainExtent,clearValues);

    auto result = vkBeginCommandBuffer(activedFrameCommandBuferToSubmit, &cmdBufferBeginInfo);
    if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
    vkCmdBeginRenderPass(activedFrameCommandBuferToSubmit, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(activedFrameCommandBuferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS ,plantPipeline);

    auto viewport = FnCommand::viewport(activedFrameCommandBuferToSubmit, simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    auto scissor = FnCommand::scissor(activedFrameCommandBuferToSubmit, simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    vkCmdSetViewport(activedFrameCommandBuferToSubmit, 0, 1, &viewport);
    vkCmdSetScissor(activedFrameCommandBuferToSubmit,0, 1, &scissor);


    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(activedFrameCommandBuferToSubmit, 0, 1, &plantGeo.verticesBuffer, offsets);
    vkCmdBindIndexBuffer(activedFrameCommandBuferToSubmit,plantGeo.indicesBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Render multiple objects using different model matrices by dynamically offsetting into one uniform buffer
    for (uint32_t j = 0; j < OBJECT_INSTANCES; j++)
    {
        // One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
        uint32_t dynamicOffset = j * static_cast<uint32_t>(dynamicAlignment);
        // Bind the descriptor set for rendering a mesh using the dynamic offset
        vkCmdBindDescriptorSets(activedFrameCommandBuferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, plantPipelineLayout, 0, 2, plantDescriptorSets, 1, &dynamicOffset);

        vkCmdDrawIndexed(activedFrameCommandBuferToSubmit, plantGeo.indices.size(), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(activedFrameCommandBuferToSubmit);
    if (vkEndCommandBuffer(activedFrameCommandBuferToSubmit) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

LLVK_NAMESPACE_END
