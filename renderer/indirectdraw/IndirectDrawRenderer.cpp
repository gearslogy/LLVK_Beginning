//
// Created by liuya on 8/16/2024.
//

#include "IndirectDrawRenderer.h"
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
#include "IndirectDrawPass.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include <ranges>

#include "renderer/instance/instance_v2/TerrainPassV2.h"
LLVK_NAMESPACE_BEGIN
    void Resources::loading() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &phyDevice = pRenderer->getMainDevice().physicalDevice;
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    loadTree();
    loadTerrain();
    loadGrass();
    loadFlower();
}

void Resources::cleanup() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geos.geoBufferManager);
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_resources(terrainTextures.albedoArray,
        terrainTextures.normalArray,
        terrainTextures.ordpArray,terrainTextures.mask );
    UT_Fn::cleanup_range_resources(treeTextures.leaves);
    UT_Fn::cleanup_range_resources(treeTextures.branch);
    UT_Fn::cleanup_range_resources(treeTextures.root);
    UT_Fn::cleanup_range_resources(grassTextures);
    UT_Fn::cleanup_range_resources(flowerTextures);

}
void Resources::loadTerrain() {
    setRequiredObjectsByRenderer(pRenderer,geos.geoBufferManager);
    geos.terrain.load("content/scene/instance/gltf/terrain_output_v2.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.terrain, geos.geoBufferManager);

    setRequiredObjectsByRenderer(pRenderer,terrainTextures.albedoArray,terrainTextures.normalArray,
        terrainTextures.ordpArray,terrainTextures.mask);
    terrainTextures.albedoArray.create("content/scene/instance/tex/terrain/gpu_albedo_2darray_v2.ktx2",colorSampler);
    terrainTextures.normalArray.create("content/scene/instance/tex/terrain/gpu_n_2darray_v2.ktx2",colorSampler);
    terrainTextures.ordpArray.create("content/scene/instance/tex/terrain/gpu_ordp_2darray_v2.ktx2",colorSampler);
    terrainTextures.mask.create("content/scene/instance/tex/terrain/gpu_terrain_mask_v2.ktx2",colorSampler);


    // scene pass prepare
    TerrainGeometryContainerV2::RenderableObject terrain;
    terrain.pGeometry = &geos.terrain.parts[0];
    terrain.bindTextures(&terrainTextures.albedoArray,
        &terrainTextures.ordpArray,
        &terrainTextures.normalArray,
        &terrainTextures.mask);
    auto &&terrainGeoContainer = pTerrainPass->getGeometryContainer();
    terrainGeoContainer.addRenderableGeometry(terrain);



}

void Resources::loadTree() {
    // tree model loader
    setRequiredObjectsByRenderer(this, geos.geoBufferManager);
    geos.tree.load("content/scene/instance/gltf/tree.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.tree, geos.geoBufferManager);
    // tree texture loader
    setRequiredObjectsByRenderer(pRenderer, treeTextures.leaves);
    setRequiredObjectsByRenderer(pRenderer, treeTextures.branch);
    setRequiredObjectsByRenderer(pRenderer, treeTextures.root);
    std::array<std::string,3> leavesPaths= {
        "content/scene/instance/tree_flower_grass/tree/gpu_leaves_D.ktx2",
        "content/scene/instance/tree_flower_grass/tree/gpu_leaves_N.ktx2",
        "content/scene/instance/tree_flower_grass/tree/gpu_leaves_M.ktx2",
    };
    std::array<std::string,3> branchPaths;
    std::array<std::string,3> rootPaths;
    auto leavesIter = leavesPaths | std::views::transform([&](auto &path){return UT_STR::replace(path,"leaves","branch") ;});
    std::ranges::copy(leavesIter, branchPaths.begin());
    auto rootIter = leavesPaths | std::views::transform([&](auto &path){return UT_STR::replace(path,"leaves","root") ;});
    std::ranges::copy(rootIter, rootPaths.begin());
    auto loadTextures = [this](Concept::is_range auto &paths, Concept::is_range auto &textures) {
        for(auto &&[path,t]: std::views::zip(paths, textures))
            t.create(path, colorSampler);
    };
    loadTextures(leavesPaths, treeTextures.leaves);
    loadTextures(branchPaths, treeTextures.branch);
    loadTextures(rootPaths, treeTextures.root);

    // loading instance points
    auto [instanceBuffer,instanceCount] = pInstancedObjectPass->loadInstanceData("content/scene/instance/scene_json/trees.json");

    // scene pass prepare
    InstanceGeometryContainer::RenderableObject treeLeaves;
    InstanceGeometryContainer::RenderableObject treeBranch;
    InstanceGeometryContainer::RenderableObject treeRoot;

    treeLeaves.pGeometry = &geos.tree.parts[0];
    treeLeaves.bindTextures(&treeTextures.leaves[0],&treeTextures.leaves[1],&treeTextures.leaves[2]);
    treeLeaves.instDesc = {instanceBuffer, instanceCount};

    treeBranch.pGeometry = &geos.tree.parts[1];
    treeBranch.bindTextures(&treeTextures.branch[0],&treeTextures.branch[1],&treeTextures.branch[2]);
    treeBranch.instDesc = {instanceBuffer, instanceCount};


    treeRoot.pGeometry = &geos.tree.parts[2];
    treeRoot.bindTextures(&treeTextures.root[0],&treeTextures.root[1],&treeTextures.root[2]);
    treeRoot.instDesc = {instanceBuffer, instanceCount};

    pInstancedObjectPass->geoContainer.addRenderableGeometry(treeLeaves, InstanceGeometryContainer::opacity);
    pInstancedObjectPass->geoContainer.addRenderableGeometry(treeBranch, InstanceGeometryContainer::opaque);
    pInstancedObjectPass->geoContainer.addRenderableGeometry(treeRoot, InstanceGeometryContainer::opaque);
}

void Resources::loadFlower() {
    setRequiredObjectsByRenderer(this, geos.geoBufferManager);
    geos.flower.load("content/scene/instance/gltf/flower.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.flower, geos.geoBufferManager);
    std::array<std::string,3> paths= {
        "content/scene/instance/tree_flower_grass/flower/gpu_D.ktx2",
        "content/scene/instance/tree_flower_grass/flower/gpu_N.ktx2",
        "content/scene/instance/tree_flower_grass/flower/gpu_M.ktx2",
    };
    setRequiredObjectsByRenderer(pRenderer,flowerTextures);
    for(auto &&[path,t]: std::views::zip(paths, flowerTextures))
        t.create(path, colorSampler);

    // loading instance points
    auto [instanceBuffer,instanceCount] = pInstancedObjectPass->loadInstanceData("content/scene/instance/scene_json/flower.json");
    InstanceGeometryContainer::RenderableObject flower;
    flower.pGeometry = &geos.flower.parts[0];
    flower.bindTextures(&flowerTextures[0],&flowerTextures[1],&flowerTextures[2]);
    flower.instDesc = {instanceBuffer, instanceCount};
    pInstancedObjectPass->geoContainer.addRenderableGeometry(flower, InstanceGeometryContainer::opacity);
}

void Resources::loadGrass() {
    setRequiredObjectsByRenderer(this, geos.geoBufferManager);
    geos.grass.load("content/scene/instance/gltf/grass.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.grass, geos.geoBufferManager);
    std::array<std::string,3> paths= {
        "content/scene/instance/tree_flower_grass/grass/gpu_D.ktx2",
        "content/scene/instance/tree_flower_grass/grass/gpu_N.ktx2",
        "content/scene/instance/tree_flower_grass/grass/gpu_M.ktx2",
    };
    setRequiredObjectsByRenderer(pRenderer,grassTextures );
    for(auto &&[path,t]: std::views::zip(paths, grassTextures))
        t.create(path, colorSampler);
    // loading instance points
    auto [instanceBuffer,instanceCount] = pInstancedObjectPass->loadInstanceData("content/scene/instance/scene_json/grass.json");
    InstanceGeometryContainer::RenderableObject grass;
    grass.pGeometry = &geos.grass.parts[0];
    grass.bindTextures(&grassTextures[0],&grassTextures[1],&grassTextures[2]);
    grass.instDesc = {instanceBuffer, instanceCount};
    pInstancedObjectPass->geoContainer.addRenderableGeometry(grass, InstanceGeometryContainer::opacity);
}


InstanceRendererV2::InstanceRendererV2() {
    mainCamera.mPosition = {6.26441,8.5,412.539};
    mainCamera.mYaw =-358.759f;
    mainCamera.mMoveSpeed =  5.0;
    mainCamera.updateCameraVectors();
    instancePass = std::make_unique<InstancePass>(this, &descPool);
    terrainPass = std::make_unique<TerrainPassV2>(this, &descPool);
}
InstanceRendererV2::~InstanceRendererV2() = default;

void InstanceRendererV2::cleanupObjects() {
    const auto && device = getMainDevice().logicalDevice;
    vkDestroyDescriptorPool(device, descPool, nullptr);
    resources.cleanup();
    instancePass->cleanup();
    terrainPass->cleanup();
}

void InstanceRendererV2::loadResources() {
    resources.pRenderer = this;
    resources.pInstancedObjectPass = instancePass.get();
    resources.pTerrainPass = terrainPass.get();
    resources.loading();
}


void InstanceRendererV2::prepare() {
    createDescriptorPool();
    loadResources();
    instancePass->prepare();
    terrainPass->prepare();
}

void InstanceRendererV2::createDescriptorPool() {
    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert(result == VK_SUCCESS);
}



void InstanceRendererV2::updateUniformBuffers() {
    auto identity = glm::mat4(1.0f);
    instancePass->updateUniformBuffers(identity, glm::vec4{lightPos,1.0f});
    terrainPass->updateUniformBuffers(identity, glm::vec4{lightPos,1.0f});
}


void InstanceRendererV2::recordCommandBuffer() {
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin shadow command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    std::vector<VkClearValue> sceneClearValues(2);
    sceneClearValues[0].color = {0.4, 0.4, 0.4, 1};
    sceneClearValues[1].depthStencil = { 1.0f, 0 };
    const auto sceneRenderPass =getMainRenderPass();
    const auto sceneRenderExtent = getSwapChainExtent();
    const auto sceneFramebuffer = getMainFramebuffer();
    const auto sceneRenderPassBeginInfo = FnCommand::renderPassBeginInfo(sceneFramebuffer, sceneRenderPass, sceneRenderExtent,sceneClearValues);

    vkCmdBeginRenderPass(cmdBuf, &sceneRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    auto [vs_width , vs_height] = sceneRenderExtent;
    auto viewport = FnCommand::viewport(vs_width,vs_height );
    auto scissor = FnCommand::scissor(vs_width, vs_height);
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);

    instancePass->recordCommandBuffer();
    terrainPass->recordCommandBuffer();


    vkCmdEndRenderPass(cmdBuf);
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}

LLVK_NAMESPACE_END
