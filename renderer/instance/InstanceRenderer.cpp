//
// Created by liuya on 8/16/2024.
//

#include "InstanceRenderer.h"
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
LLVK_NAMESPACE_BEGIN
void InstanceRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_resources(geos.geoBufferManager);
    UT_Fn::cleanup_resources(Textures.groundCliff,Textures.groundGrass, Textures.groundGrass );
    UT_Fn::cleanup_sampler(device,colorSampler);
    vkDestroyDescriptorPool(device, descPool, nullptr);
}

void InstanceRenderer::loadTexture() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    setRequiredObjects(Textures.groundCliff, Textures.groundGrass,Textures.groundRock);
    Textures.groundCliff.create("content/scene/instance/tex/green_cliff_texarray.ktx2",colorSampler);
    Textures.groundGrass.create("content/scene/instance/tex/green_grass_texarray.ktx2",colorSampler);
    Textures.groundRock.create("content/scene/instance/tex/green_rock_texarray.ktx2",colorSampler);
}

void InstanceRenderer::loadModel() {
    setRequiredObjects(geos.geoBufferManager);
    geos.terrain.load("content/scene/instance/gltf/terrain_output.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.terrain, Geos.geoBufferManager);
}
void InstanceRenderer::prepare() {
    loadTexture();
    loadModel();
    createDescriptorPool();
    prepareUniformBuffers();
}

void InstanceRenderer::createDescriptorPool() {
    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert(result == VK_SUCCESS);
}


void InstanceRenderer::preparePipelines() {
}

void InstanceRenderer::prepareUniformBuffers() {
}

void InstanceRenderer::updateUniformBuffers() {
}


void InstanceRenderer::recordCommandBuffer() {

}

LLVK_NAMESPACE_END
