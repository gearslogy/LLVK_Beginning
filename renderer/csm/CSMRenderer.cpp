//
// Created by liuya on 11/8/2024.
//

#include "CSMRenderer.h"
#include "CSMPass.h"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
    void CSMRenderer::ResourceManager::loading() {
    setRequiredObjectsByRenderer(pRenderer, geos.geometryBufferManager);
    setRequiredObjectsByRenderer(pRenderer, textures.d_tex_29, textures.d_tex_35,
        textures.d_tex_39,textures.d_tex_36, textures.d_ground);
    geos.ground.load("content/scene/csm/resources/gpu_models/ground.gltf");
    geos.geo_29.load("content/scene/csm/resources/gpu_models/29_WatchTower.gltf");
    geos.geo_35.load("content/scene/csm/resources/gpu_models/35_MedBuilding.gltf");
    geos.geo_36.load("content/scene/csm/resources/gpu_models/36_MedBuilding.gltf");
    geos.geo_36.load("content/scene/csm/resources/gpu_models/39_MedBuilding.gltf");



}

void CSMRenderer::ResourceManager::cleanup() {
    UT_Fn::cleanup_resources(geos.geometryBufferManager,textures.d_ground, textures.d_tex_29,
        textures.d_tex_35, textures.d_tex_36,textures.d_tex_39);

}






LLVK_NAMESPACE_END
