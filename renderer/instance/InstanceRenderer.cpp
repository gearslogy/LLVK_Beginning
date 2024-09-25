//
// Created by liuya on 8/16/2024.
//

#include "InstanceRenderer.h"
#include "LLVK_UT_VmaBuffer.hpp"
LLVK_NAMESPACE_BEGIN
void InstanceRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_resources(Geos.geoBufferManager);
    UT_Fn::cleanup_resources(Textures.greenPlant, Textures.yellowPlant, Textures.largePlant );
    UT_Fn::cleanup_resources(Textures.groundCliff,Textures.groundGrass, Textures.groundGrass );

    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    //UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);
    UT_Fn::cleanup_sampler(device,colorSampler);

    vkDestroyDescriptorPool(device, pool, nullptr);
}

void InstanceRenderer::loadTexture() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    setRequiredObjects(Textures.greenPlant, Textures.yellowPlant,Textures.largePlant);
    setRequiredObjects(Textures.groundCliff, Textures.groundGrass,Textures.groundRock);
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    Textures.greenPlant.create("content/scene/instance/tex/green_grass_texarray.ktx2",colorSampler);
    Textures.yellowPlant.create("content/scene/instance/tex/yellow_grass_texarray.ktx2",colorSampler);
    Textures.largePlant.create("content/scene/instance/tex/large_foliage_texarray.ktx2",colorSampler);
    Textures.groundCliff.create("content/scene/instance/tex/green_cliff_texarray.ktx2",colorSampler);
    Textures.groundGrass.create("content/scene/instance/tex/green_grass_texarray.ktx2",colorSampler);
    Textures.groundRock.create("content/scene/instance/tex/green_rock_texarray.ktx2",colorSampler);
}

void InstanceRenderer::loadModel() {
    setRequiredObjects(Geos.geoBufferManager);
    Geos.greenPlantGeo.load("content/scene/instance/gltf/green_grass_output.gltf");
    Geos.yellowPlantGeo.load("content/scene/instance/gltf/yellow_foliage_output.gltf");
    Geos.largePlantGeo.load("content/scene/instance/gltf/large_foliage_output.gltf");
    Geos.groundGeo.load("content/ground/ground.gltf");

    UT_VmaBuffer::addGeometryToSimpleBufferManager(Geos.greenPlantGeo, Geos.geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(Geos.yellowPlantGeo, Geos.geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(Geos.largePlantGeo, Geos.geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(Geos.groundGeo, Geos.geoBufferManager);
}

void InstanceRenderer::setupDescriptors() {

}


void InstanceRenderer::preparePipelines() {
}

void InstanceRenderer::prepareUniformBuffers() {
}

void InstanceRenderer::updateUniformBuffers() {
}

void InstanceRenderer::bindResources() {

}

void InstanceRenderer::recordCommandBuffer() {

}

LLVK_NAMESPACE_END
