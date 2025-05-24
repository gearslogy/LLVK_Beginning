//
// Created by liuya on 9/1/2024.
//
#include "ShadowMapPass.h"
#include "Shadowmap_v2.h"
#include <libs/magic_enum.hpp>
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
#include "Pipeline.hpp"
#include "ScenePass.h"

LLVK_NAMESPACE_BEGIN
    Shadowmap_v2::Shadowmap_v2() {
    mainCamera.mPosition = glm::vec3(15,20,192);
    const VkCommandBuffer &cmdBuf = getMainCommandBuffer();
    shadowMapPass = std::make_unique<ShadowMapPass>(this,
        &descPool,
        &cmdBuf);
    scenePass = std::make_unique<ScenePass>(this,&descPool);
    lightPos = {281.654, 120,316.942};
}
Shadowmap_v2::~Shadowmap_v2() = default;



void Shadowmap_v2::render() {
    updateUniformBuffers();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}

void Shadowmap_v2::cleanupObjects() {
    auto device = mainDevice.logicalDevice;
    shadowMapPass->cleanup();
    scenePass->cleanup();
    vkDestroyDescriptorPool(device, descPool, nullptr);
    UT_Fn::cleanup_resources(geoBufferManager, foliageAlbedoTex, foliageOrdpTex);
    UT_Fn::cleanup_resources(gridAlbedoTex, gridOrdpTex);
    UT_Fn::cleanup_sampler(device, colorSampler);
}

void Shadowmap_v2::prepare() {
    colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    loadTextures();
    loadModels();
    createDescriptorPool();
    // shadow pass prepare
    shadowMapPass->lightPos = lightPos;
    auto &&geoContainer = shadowMapPass->getGeometryContainer();
    ShadowMapGeometryContainer::RenderableObject smRenderObjs[2];
    smRenderObjs[0].pGeometry = &gridGeo.parts[0];
    smRenderObjs[0].pTexture  = &gridAlbedoTex;
    smRenderObjs[1].pGeometry = &foliageGeo.parts[0];
    smRenderObjs[1].pTexture  = &foliageAlbedoTex;
    geoContainer.addRenderableGeometry(smRenderObjs[0]);
    geoContainer.addRenderableGeometry(smRenderObjs[1]);
    shadowMapPass->prepare();

    // scene pass prepare
    SceneGeometryContainer::RenderableObject sceneRenderObjs[2];
    sceneRenderObjs[0].pGeometry = &foliageGeo.parts[0];
    sceneRenderObjs[0].pTextures.emplace_back(&foliageAlbedoTex );
    sceneRenderObjs[0].pTextures.emplace_back(&foliageOrdpTex);
    sceneRenderObjs[0].pTextures.emplace_back(&shadowMapPass->shadowFramebuffer.depthAttachment );

    sceneRenderObjs[1].pGeometry = &gridGeo.parts[0];
    sceneRenderObjs[1].pTextures.emplace_back(&gridAlbedoTex );
    sceneRenderObjs[1].pTextures.emplace_back(&gridOrdpTex);
    sceneRenderObjs[1].pTextures.emplace_back(&shadowMapPass->shadowFramebuffer.depthAttachment);
    auto &&sceneGeoContainer = scenePass->getGeometryContainer();
    sceneGeoContainer.addRenderableGeometry(sceneRenderObjs[0], SceneGeometryContainer::ObjectPipelineTag::opacity);
    sceneGeoContainer.addRenderableGeometry(sceneRenderObjs[1], SceneGeometryContainer::ObjectPipelineTag::opaque);
    // scene pass prepare
    scenePass->prepare();
}


void Shadowmap_v2::loadTextures() {
    setRequiredObjects(foliageAlbedoTex, foliageOrdpTex,gridAlbedoTex,gridOrdpTex);
    foliageAlbedoTex.create("content/shadowmap_v2/plant/basecolor.ktx2", colorSampler );
    foliageOrdpTex.create("content/shadowmap_v2/plant/ordp.ktx2",colorSampler);
    gridAlbedoTex.create("content/shadowmap_v2/ground/basecolor.ktx2", colorSampler );
    gridOrdpTex.create("content/shadowmap_v2/ground/ordp.ktx2", colorSampler );
}

void Shadowmap_v2::loadModels() {
    setRequiredObjects(geoBufferManager);
    gridGeo.load("content/shadowmap/grid.gltf");
    foliageGeo.load("content/plants/gardenplants/var0.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(gridGeo, geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(foliageGeo, geoBufferManager);
}


void Shadowmap_v2::createDescriptorPool() {
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

void Shadowmap_v2::updateUniformBuffers() {
    shadowMapPass->updateUniformBuffers();
    scenePass->updateUniformBuffers(shadowMapPass->depthMVP, glm::vec4{lightPos, 1.0});
}

void Shadowmap_v2::recordCommandBuffer() {
    //vkResetCommandBuffer(activatedFrameCommandBufferToSubmit,/*VkCommandBufferResetFlagBits*/ 0); //0: command buffer reset
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = getMainCommandBuffer();
    UT_Fn::invoke_and_check("begin shadow command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    shadowMapPass->recordCommandBuffer();
    scenePass->recordCommandBuffer();
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}




LLVK_NAMESPACE_END
