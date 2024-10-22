//
// Created by liuya on 8/16/2024.
//

#include "InstanceRenderer.h"
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
LLVK_NAMESPACE_BEGIN
InstanceRenderer::InstanceRenderer() {
    mainCamera.mPosition = {-411,59,523};
    mainCamera.mYaw = -37.0f;
    mainCamera.mMoveSpeed =  5.0;
    mainCamera.updateCameraVectors();

    terrainPass = std::make_unique<TerrainPass>(this, &descPool);

}
InstanceRenderer::~InstanceRenderer() = default;

void InstanceRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_resources(geos.geoBufferManager);
    UT_Fn::cleanup_resources(terrainTextures.albedoArray,
        terrainTextures.normalArray,
        terrainTextures.ordpArray,terrainTextures.mask );
    UT_Fn::cleanup_sampler(device,colorSampler);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    terrainPass->cleanup();
}

void InstanceRenderer::loadTexture() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    setRequiredObjects(terrainTextures.albedoArray,
        terrainTextures.normalArray,
        terrainTextures.ordpArray,
        terrainTextures.mask);
    terrainTextures.albedoArray.create("content/scene/instance/tex/terrain/gpu_albedo_2darray.ktx2",colorSampler);
    terrainTextures.normalArray.create("content/scene/instance/tex/terrain/gpu_n_2darray.ktx2",colorSampler);
    terrainTextures.ordpArray.create("content/scene/instance/tex/terrain/gpu_ordp_2darray.ktx2",colorSampler);
    terrainTextures.mask.create("content/scene/instance/tex/terrain/gpu_terrain_mask.ktx2",colorSampler);
}
void InstanceRenderer::loadModel() {
    setRequiredObjects(geos.geoBufferManager);
    geos.terrain.load("content/scene/instance/gltf/terrain_output.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.terrain, geos.geoBufferManager);
}


void InstanceRenderer::prepare() {
    loadTexture();
    loadModel();
    createDescriptorPool();

    // scene pass prepare
    TerrainGeometryContainer::RenderableObject terrain;
    terrain.pGeometry = &geos.terrain.parts[0];
    terrain.bindTextures(&terrainTextures.albedoArray,
        &terrainTextures.ordpArray,
        &terrainTextures.normalArray,
        &terrainTextures.mask);
    auto &&terrainGeoContainer = terrainPass->getGeometryContainer();
    terrainGeoContainer.addRenderableGeometry(terrain);
    terrainPass->prepare();
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



void InstanceRenderer::updateUniformBuffers() {
    auto identity = glm::mat4(1.0f);
    terrainPass->updateUniformBuffers(identity, glm::vec4{lightPos,1.0f});
}


void InstanceRenderer::recordCommandBuffer() {
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin shadow command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    terrainPass->recordCommandBuffer();
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}

LLVK_NAMESPACE_END
