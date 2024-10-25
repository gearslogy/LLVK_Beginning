//
// Created by liuya on 8/16/2024.
//

#include "InstanceRendererV2.h"
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
#include "JsonSceneParser.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include <ranges>
LLVK_NAMESPACE_BEGIN

void Resources::loading() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &phyDevice = pRenderer->getMainDevice().physicalDevice;
    colorSampler = FnImage::createImageSampler(phyDevice, device);
}

void Resources::cleanup() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geos.geoBufferManager);
    UT_Fn::cleanup_resources(terrainTextures.albedoArray,
        terrainTextures.normalArray,
        terrainTextures.ordpArray,terrainTextures.mask );

}
void Resources::loadTerrain() {
    setRequiredObjectsByRenderer(this,geos.geoBufferManager);
    geos.terrain.load("content/scene/instance/gltf/terrain_output_v2.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.terrain, geos.geoBufferManager);

    setRequiredObjectsByRenderer(pRenderer,terrainTextures.albedoArray,terrainTextures.normalArray,
        terrainTextures.ordpArray,terrainTextures.mask);
    terrainTextures.albedoArray.create("content/scene/instance/tex/terrain/gpu_albedo_2darray_v2.ktx2",colorSampler);
    terrainTextures.normalArray.create("content/scene/instance/tex/terrain/gpu_n_2darray_v2.ktx2",colorSampler);
    terrainTextures.ordpArray.create("content/scene/instance/tex/terrain/gpu_ordp_2darray_v2.ktx2",colorSampler);
    terrainTextures.mask.create("content/scene/instance/tex/terrain/gpu_terrain_mask.ktx2_v2",colorSampler);
}

void Resources::loadTree() {
    // tree model loader
    setRequiredObjectsByRenderer(this,geos.geoBufferManager);
    geos.tree.load("content/scene/instance/gltf/tree.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.tree, geos.geoBufferManager);
    // tree texture loader
    setRequiredObjectsByRenderer(this, treeTextures.leaves);
    setRequiredObjectsByRenderer(this, treeTextures.branch);
    setRequiredObjectsByRenderer(this, treeTextures.root);
    std::array<std::string,3> leavesPaths= {
        "content/scene/instance/tree_flower_grass/tree/gpu_leaves_D",
        "content/scene/instance/tree_flower_grass/tree/gpu_leaves_N",
        "content/scene/instance/tree_flower_grass/tree/gpu_leaves_M",
    };
    std::array<std::string,3> branchPaths;
    std::array<std::string,3> rootPaths;

    auto leavesIter = leavesPaths | std::views::transform([&](auto &path){return UT_STR::replace(path,"leaves","branch") ;});
    std::ranges::copy(leavesIter, branchPaths.begin());
    auto rootIter = leavesPaths | std::views::transform([&](auto &path){return UT_STR::replace(path,"leaves","root") ;});
    std::ranges::copy(rootIter, rootPaths.begin());


}





InstanceRendererV2::InstanceRendererV2() {
    mainCamera.mPosition = {-411,59,523};
    mainCamera.mYaw = -37.0f;
    mainCamera.mMoveSpeed =  5.0;
    mainCamera.updateCameraVectors();
    instancePass = std::make_unique<InstancedObjectPass>(this, &descPool);
}
InstanceRendererV2::~InstanceRendererV2() = default;

void InstanceRendererV2::cleanupObjects() {
    const auto && device = getMainDevice().logicalDevice;
    vkDestroyDescriptorPool(device, descPool, nullptr);
    resources.cleanup();
    instancePass->cleanup();
}

void InstanceRendererV2::loadTexture() {

}
void InstanceRendererV2::loadModel() {

}


void InstanceRendererV2::prepare() {
    resources.pRenderer = this;
    resources.loading();
    createDescriptorPool();

    // scene pass prepare
    InstanceGeometryContainer::RenderableObject terrain;
    terrain.pGeometry = &geos.terrain.parts[0];
    terrain.bindTextures(&terrainTextures.albedoArray,
        &terrainTextures.ordpArray,
        &terrainTextures.normalArray,
        &terrainTextures.mask);
    auto &&terrainGeoContainer = instancePass->getGeometryContainer();
    terrainGeoContainer.addRenderableGeometry(terrain);
    instancePass->prepare();
}

void InstanceRendererV2::createDescriptorPool() {
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



void InstanceRendererV2::updateUniformBuffers() {
    auto identity = glm::mat4(1.0f);
    terrainPass->updateUniformBuffers(identity, glm::vec4{lightPos,1.0f});
}


void InstanceRendererV2::recordCommandBuffer() {
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin shadow command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    terrainPass->recordCommandBuffer();
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}

LLVK_NAMESPACE_END
