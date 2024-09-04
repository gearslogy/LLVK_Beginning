//
// Created by liuya on 9/1/2024.
//

#include "shadowmap.h"
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
LLVK_NAMESPACE_BEGIN
void shadowmap::render() {
}

void shadowmap::cleanupObjects() {
    auto device = mainDevice.logicalDevice;
    vkDestroySampler(device, sampler, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    geoBufferManager.cleanup();
    foliageTex.cleanup();
    gridTex.cleanup();
    UT_Fn::cleanup_pipeline(device, pipelines.sceneOpacity, pipelines.sceneOpaque);
    UT_Fn::cleanup_pipeline(device, pipelines.offscreenOpacity, pipelines.offscreenOpaque);
    vkDestroyPipelineLayout(device, pipelines.layout, nullptr);

}

void shadowmap::prepare() {
    loadTextures();
    loadModels();
}

void shadowmap::loadTextures() {
    sampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    setRequiredObjects(foliageTex, gridTex);
    foliageTex.create("content/plants/gardenplants/tex_array.ktx2", sampler );
    gridTex.create("content/shadowmap/grid_tex/tex_array.ktx2",sampler);
}

void shadowmap::loadModels() {
    setRequiredObjects(geoBufferManager);
    gridGeo.load("content/shadowmap/grid.gltf");
    foliageGeo.load("content/plants/gardenplants/var0.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(gridGeo, geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(foliageGeo, geoBufferManager);
}


void shadowmap::prepareOffscreen() {

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
    auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT); // maps
    auto set0_ubo_binding2 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_VERTEX_BIT); // depth
    const std::array set0_bindings = {set0_ubo_binding0, set0_ubo_binding1, set0_ubo_binding2};

    const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(set0_bindings);
    if(vkCreateDescriptorSetLayout(device, &setLayoutCIO, nullptr, &descriptorSets.descriptorSetLayout)!=VK_SUCCESS)
        throw std::runtime_error{"Error create plant ubo set layout"};
    std::array layouts = {descriptorSets.descriptorSetLayout};

    // create set
    auto allocInfo = FnDescriptor::setAllocateInfo(descPool,layouts);
    UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &allocInfo,descriptorSets.offscreenOpacity );
    UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &allocInfo,descriptorSets.offscreenOpaque );
    UT_Fn::invoke_and_check("Error create scene sets",vkAllocateDescriptorSets,device, &allocInfo,descriptorSets.sceneOpacity );
    UT_Fn::invoke_and_check("Error create scene sets",vkAllocateDescriptorSets,device, &allocInfo,descriptorSets.sceneOpaque );
    // write set
    // - offscreen opacity
    std::vector<VkWriteDescriptorSet> opacityWriteSets;
    opacityWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.offscreenOpacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.offscreen.descBufferInfo));
    opacityWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.offscreenOpacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1, &foliageTex.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opacityWriteSets.size()),opacityWriteSets.data(),0, nullptr);
    // - offscreen opaque
    std::vector<VkWriteDescriptorSet> opaqueWriteSets;
    opaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.offscreenOpaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.offscreen.descBufferInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opaqueWriteSets.size()),opaqueWriteSets.data(),0, nullptr);

    // - scene opacity
    std::vector<VkWriteDescriptorSet> opacitySceneWriteSets;
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.sceneOpacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.scene.descBufferInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.sceneOpacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1, &foliageTex.descImageInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.sceneOpacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2, &shadowFramebuffer.depthAttachment.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opacitySceneWriteSets.size()),opacitySceneWriteSets.data(),0, nullptr);

    // - scene opaque
    std::vector<VkWriteDescriptorSet> sceneOpaqueWriteSets;
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.sceneOpaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.scene.descBufferInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.sceneOpaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1, &gridTex.descImageInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(descriptorSets.sceneOpaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2, &shadowFramebuffer.depthAttachment.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(sceneOpaqueWriteSets.size()),sceneOpaqueWriteSets.data(),0, nullptr);
}



void shadowmap::preparePipelines() {

}



LLVK_NAMESPACE_END
