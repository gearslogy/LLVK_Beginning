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
void IndirectResources::loading() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &phyDevice = pRenderer->getMainDevice().physicalDevice;
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    loadTerrain();
    loadFoliage();
}

void IndirectResources::cleanup() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geos.geoBufferManager, combinedObject.bufferManager);
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_resources(terrainTextures.albedoArray,
        terrainTextures.normalArray,
        terrainTextures.ordpArray,terrainTextures.mask );
    UT_Fn::cleanup_resources(foliageD_array, foliageN_array, foliageM_Array);
}
void IndirectResources::loadTerrain() {
    setRequiredObjectsByRenderer(pRenderer,geos.geoBufferManager);
    geos.terrain.load("content/scene/indirectdraw/gltf/terrain.gltf");
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


void IndirectResources::loadFoliage() {
    // tree model loader
    geos.grass.load("content/scene/indirectdraw/gltf/grass.gltf");
    geos.flower.load("content/scene/indirectdraw/gltf/flower.gltf");
    geos.tree.load("content/scene/indirectdraw/gltf/tree.gltf");

    // tree texture loader
    setRequiredObjectsByRenderer(pRenderer, combinedObject.bufferManager);
    setRequiredObjectsByRenderer(pRenderer, foliageD_array);
    setRequiredObjectsByRenderer(pRenderer, foliageN_array);
    setRequiredObjectsByRenderer(pRenderer, foliageM_Array);

    foliageD_array.create("content/scene/indirectdraw/tex/gpu_D_array.ktx2", colorSampler);
    foliageN_array.create("content/scene/indirectdraw/tex/gpu_N_array.ktx2",colorSampler);
    foliageM_Array.create("content/scene/indirectdraw/tex/gpu_M_array.ktx2",colorSampler);

    IndirectDrawPass::JsonPointsParser grassParser{"content/scene/indirectdraw/scene_json/grass.json"};// grass
    IndirectDrawPass::JsonPointsParser flowerParser{"content/scene/indirectdraw/scene_json/flower.json"};// flower
    IndirectDrawPass::JsonPointsParser treeParser{"content/scene/indirectdraw/scene_json/trees.json"}; // tree
    std::cout << "we will instance points num: " << grassParser.instanceData.size() + flowerParser.instanceData.size() + treeParser.instanceData.size()*3 << std::endl;
    std::vector<uint32_t> instanceCount(5);
    instanceCount[0] = grassParser.instanceData.size();
    instanceCount[1] = flowerParser.instanceData.size();
    instanceCount[2] = treeParser.instanceData.size();
    instanceCount[3] = treeParser.instanceData.size();
    instanceCount[4] = treeParser.instanceData.size();


    std::vector<IndirectDrawPass::InstanceData> allInstanceData;
    allInstanceData.reserve(grassParser.instanceData.size() + flowerParser.instanceData.size() + treeParser.instanceData.size()*3);

    std::ranges::for_each(grassParser.instanceData,[](auto &instanceData) {instanceData.texId = 0;}); // grass
    allInstanceData.insert(allInstanceData.end(), std::make_move_iterator(grassParser.instanceData.begin()), std::make_move_iterator( grassParser.instanceData.end()));
    std::ranges::for_each(flowerParser.instanceData,[](auto &instanceData) {instanceData.texId = 1;}); // flower
    allInstanceData.insert(allInstanceData.end(), std::make_move_iterator(flowerParser.instanceData.begin()), std::make_move_iterator(flowerParser.instanceData.end()));

    std::ranges::for_each(treeParser.instanceData, [&allInstanceData](auto &instanceData) {
        instanceData.texId = 2;
        allInstanceData.emplace_back(instanceData);
    });// tree-leaves
    std::ranges::for_each(treeParser.instanceData, [&allInstanceData](auto &instanceData) {
        instanceData.texId = 3;
        allInstanceData.emplace_back(instanceData);
     });// tree-branch
    std::ranges::for_each(treeParser.instanceData, [&allInstanceData](auto &instanceData) {
        instanceData.texId = 4;
        allInstanceData.emplace_back(instanceData);
    });// tree-root

    // 1. INSTANCE BUFFER
    combinedObject.bufferManager.createBufferWithStagingBuffer<VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT>( sizeof(IndirectDrawPass::InstanceData) * allInstanceData.size(),
        allInstanceData.data());
    VkBuffer instanceBuffer =  combinedObject.bufferManager.createVertexBuffers.back().buffer; // <INSTANCE BUFFER>

    // 2. whole geo buffer
    combinedObject.combinedGeometry.parts = {
        &geos.grass.parts[0],
        &geos.flower.parts[0],
        &geos.tree.parts[0],
        &geos.tree.parts[1],
        &geos.tree.parts[2],
    };
    combinedObject.combinedGeometry.computeCombinedData();

    VkDeviceSize vertexBufferSize =  combinedObject.combinedGeometry.vertices.size() * sizeof(GLTFVertex);
    VkDeviceSize indicesBufferSize =  combinedObject.combinedGeometry.indices.size() * sizeof(uint32_t);
    combinedObject.bufferManager.createBufferWithStagingBuffer<VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT>(vertexBufferSize,  combinedObject.combinedGeometry.vertices.data());
    VkBuffer verticesBuffer = combinedObject.bufferManager.createVertexBuffers.back().buffer;
    combinedObject.bufferManager.createBufferWithStagingBuffer<VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT>(indicesBufferSize,  combinedObject.combinedGeometry.indices.data());
    VkBuffer indicesBuffer = combinedObject.bufferManager.createIndexedBuffers.back().buffer;

    //3 Indirect Command
    std::vector<VkDrawIndexedIndirectCommand> indirectCommands;
    int m = 0;
    int instanceOffset = 0;
    for(auto *part :  combinedObject.combinedGeometry.parts) {
        VkDrawIndexedIndirectCommand indirectCmd{};
        indirectCmd.instanceCount = instanceCount[m];
        indirectCmd.firstInstance = instanceOffset;
        indirectCmd.firstIndex = part->firstIndex;
        indirectCmd.indexCount = part->indices.size();
        indirectCommands.emplace_back(indirectCmd);

        instanceOffset += instanceCount[m];
        m++;
    }
    auto indirectDrawCount = static_cast<uint32_t>(indirectCommands.size()); //
    std::cout << "indirectDrawCount:" << indirectDrawCount << std::endl;

    uint32_t objectCount = 0;
    for (auto indirectCmd : indirectCommands) {
        objectCount += indirectCmd.instanceCount;
    }
    std::cout << "indirect draw object count:" << objectCount << std::endl;
    // 4 build indirect command buffer
    VkDeviceSize indirectCommandBufferSize = indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand);
    combinedObject.bufferManager.createBufferWithStagingBuffer<VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT>(indirectCommandBufferSize, indirectCommands.data());
    VkBuffer indirectCommandBuffer = combinedObject.bufferManager.createIndirectBuffers.back().buffer;

    // back to our IndirectDrawPass
    pIndirectDrawPass->verticesBuffer = verticesBuffer;
    pIndirectDrawPass->indicesBuffer = indicesBuffer;
    pIndirectDrawPass->instanceBuffer = instanceBuffer;
    pIndirectDrawPass->indirectCommandBuffer = indirectCommandBuffer;
    pIndirectDrawPass->indirectDrawCount = indirectDrawCount;
    pIndirectDrawPass->pTextures.emplace_back(&foliageD_array);
    pIndirectDrawPass->pTextures.emplace_back(&foliageN_array);
    pIndirectDrawPass->pTextures.emplace_back(&foliageM_Array);
}



IndirectRenderer::IndirectRenderer() {
    mainCamera.mPosition = {6.26441,8.5,412.539};
    mainCamera.mYaw =-358.759f;
    mainCamera.mMoveSpeed =  5.0;
    mainCamera.updateCameraVectors();
    indirectDrawPass = std::make_unique<IndirectDrawPass>(this, &descPool);
    terrainPass = std::make_unique<TerrainPassV2>(this, &descPool);
}
IndirectRenderer::~IndirectRenderer() = default;

void IndirectRenderer::cleanupObjects() {
    const auto && device = getMainDevice().logicalDevice;
    vkDestroyDescriptorPool(device, descPool, nullptr);
    resources.cleanup();
    indirectDrawPass->cleanup();
    terrainPass->cleanup();
}

void IndirectRenderer::loadResources() {
    resources.pRenderer = this;
    resources.pIndirectDrawPass = indirectDrawPass.get();
    resources.pTerrainPass = terrainPass.get();
    resources.loading();
}


void IndirectRenderer::prepare() {
    createDescriptorPool();
    loadResources();
    indirectDrawPass->prepare();
    terrainPass->prepare();
}

void IndirectRenderer::createDescriptorPool() {
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



void IndirectRenderer::updateUniformBuffers() {
    auto identity = glm::mat4(1.0f);
    indirectDrawPass->updateUniformBuffers(identity, glm::vec4{lightPos,1.0f});
    terrainPass->updateUniformBuffers(identity, glm::vec4{lightPos,1.0f});
}


void IndirectRenderer::recordCommandBuffer() {
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

    indirectDrawPass->recordCommandBuffer();
    terrainPass->recordCommandBuffer();


    vkCmdEndRenderPass(cmdBuf);
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}

LLVK_NAMESPACE_END
