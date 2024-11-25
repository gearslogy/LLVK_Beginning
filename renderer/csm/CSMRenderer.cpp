//
// Created by liuya on 11/8/2024.
//

#include "CSMRenderer.h"
#include "CSMPass.h"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
void CSMRenderer::ResourceManager::loading() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &logicDevice = pRenderer->getMainDevice().physicalDevice;
    setRequiredObjectsByRenderer(pRenderer, geos.geometryBufferManager);
    setRequiredObjectsByRenderer(pRenderer, textures.d_tex_29, textures.d_tex_35,
        textures.d_tex_39,textures.d_tex_36, textures.d_ground);
    geos.ground.load("content/scene/csm/resources/gpu_models/ground.gltf");
    geos.geo_29.load("content/scene/csm/resources/gpu_models/29_WatchTower.gltf");
    geos.geo_35.load("content/scene/csm/resources/gpu_models/35_MedBuilding.gltf");
    geos.geo_36.load("content/scene/csm/resources/gpu_models/36_MedBuilding.gltf");
    geos.geo_36.load("content/scene/csm/resources/gpu_models/39_MedBuilding.gltf");
    colorSampler =  FnImage::createImageSampler(logicDevice, device);
    textures.d_ground.create("content/scene/csm/resources/gpu_textures/ground_gpu_D.ktx2",colorSampler);
    textures.d_tex_29.create("content/scene/csm/resources/gpu_textures/29_WatchTower_gpu_D.ktx2",colorSampler);
    textures.d_tex_35.create("content/scene/csm/resources/gpu_textures/35_MedBuilding_gpu_D.ktx2",colorSampler);
    textures.d_tex_36.create("content/scene/csm/resources/gpu_textures/36_MedBuilding_gpu_D.ktx2",colorSampler);
    textures.d_tex_39.create("content/scene/csm/resources/gpu_textures/39_MedBuilding_gpu_D.ktx2",colorSampler);

}

CSMRenderer::ResourceManager::~ResourceManager() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geos.geometryBufferManager,textures.d_ground, textures.d_tex_29,
     textures.d_tex_35, textures.d_tex_36,textures.d_tex_39);
    UT_Fn::cleanup_sampler(device, colorSampler);

}




LLVK_NAMESPACE_END
