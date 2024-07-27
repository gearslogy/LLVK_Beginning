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
    // plant texture clean up
    for(const auto &obj : plantsImageViews) // view
        vkDestroyImageView(device, obj, nullptr);
    for(auto &[image,memory,_] : plantsImageMems) { // image and memmory
        std::cout<< "free:" <<  image << "  mem:" << memory << std::endl;
        vkDestroyImage(device, image, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }
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


void DynamicsUBO::createTextures(const Concept::is_range auto &files) {

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
    auto createTextureAndMemory = [this](const auto &file) {
        return FnImage::createTexture(mainDevice.physicalDevice, mainDevice.logicalDevice, graphicsCommandPool, mainDevice.graphicsQueue,file);
    };

    auto createImageView = [this](const ImageAndMemory &rhs) {
        return FnImage::createImageView( mainDevice.logicalDevice, rhs.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, rhs.mipLevels);
    };

    // create image and device_mem
    auto plant_image_mems = files | std::views::transform(createTextureAndMemory);
    std::ranges::copy(plant_image_mems, this->plantsImageMems.begin());
    // create image views
    auto image_views = plantsImageMems | std::views::transform(createImageView);
    std::ranges::copy(image_views, this->plantsImageViews.begin());

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
    auto createTextureAndMemory = [this](const auto &file) {
        return FnImage::createTexture(mainDevice.physicalDevice, mainDevice.logicalDevice, graphicsCommandPool, mainDevice.graphicsQueue,file);
    };

    auto createImageView = [this](const ImageAndMemory &rhs) {
        return FnImage::createImageView( mainDevice.logicalDevice, rhs.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, rhs.mipLevels);
    };

    // create image and device_mem
    auto plant_image_mems = files | std::views::transform(createTextureAndMemory);
    std::ranges::copy(plant_image_mems, this->plantsImageMems.begin());
    // create image views
    auto image_views = plantsImageMems | std::views::transform(createImageView);
    std::ranges::copy(image_views, this->plantsImageViews.begin());

}

void DynamicsUBO::loadTexture() {
    const auto phyDevice = mainDevice.physicalDevice;
    const auto device = mainDevice.logicalDevice;
    sampler = FnImage::createImageSampler(phyDevice, device); // create sampler
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


    // texture. shared sampler!
    auto createDescritporImageInfo = [this](const VkImageView &imageView)->VkDescriptorImageInfo {
        VkDescriptorImageInfo ret{};
        ret.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ret.imageView = imageView;
        ret.sampler = sampler;
        return ret;
    };
    std::array<VkDescriptorImageInfo,plantNumPlantsImages> plantDescriptorImageInfos{};
    auto iter_plantDescriptorImageInfos = xrange(0,plantDescriptorImageInfos) |
        std::views::transform([this](auto idx){return plantsImageViews[idx];}) | std::views::transform(createDescritporImageInfo);
    std::ranges::copy(iter_plantDescriptorImageInfos, plantDescriptorImageInfos.begin());

    // dynamics descritpor buffer info should modify
    plantUniformBuffers.dynamicBuffer.descBufferInfo.range = dynamicAlignment; // ! important !

    // -- write set end--
    const std::array writeSets{
        FnDesc::writeDescriptorSet(plantDescriptorSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &plantUniformBuffers.viewBuffer.descBufferInfo ), // set=0 binding=0
        FnDesc::writeDescriptorSet(plantDescriptorSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, &plantUniformBuffers.dynamicBuffer.descBufferInfo ),// set=0 binding=1
        FnDesc::writeDescriptorSet(plantDescriptorSets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &plantDescriptorImageInfos[0] ),// set=1 binding=0    };
        FnDesc::writeDescriptorSet(plantDescriptorSets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &plantDescriptorImageInfos[1] ),// set=1 binding=1    };
        FnDesc::writeDescriptorSet(plantDescriptorSets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &plantDescriptorImageInfos[2] ),// set=1 binding=2    };
        FnDesc::writeDescriptorSet(plantDescriptorSets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &plantDescriptorImageInfos[3] ),// set=1 binding=3    };
        FnDesc::writeDescriptorSet(plantDescriptorSets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &plantDescriptorImageInfos[4] ),// set=1 binding=4    };
        FnDesc::writeDescriptorSet(plantDescriptorSets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &plantDescriptorImageInfos[5] ),// set=1 binding=5    };
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

}
void DynamicsUBO::preparePipelines() {
    auto device = mainDevice.logicalDevice;
    const auto vertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/dynamicsUBO_vert.spv",  device);
    const auto fragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/dynamicsUBO_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule);
    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule);
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
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
}

void DynamicsUBO::prepareUniformBuffers() {
    auto device = mainDevice.logicalDevice;
    auto phyDevice = mainDevice.physicalDevice;
    plantUniformBuffers.viewBuffer.bindDevice = device;
    plantUniformBuffers.viewBuffer.bindPhysicalDevice = phyDevice;
    plantUniformBuffers.dynamicBuffer.bindDevice = device;
    plantUniformBuffers.dynamicBuffer.bindPhysicalDevice = phyDevice;

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
