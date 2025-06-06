//
// Created by liuya on 1/1/2025.
//

#ifndef SPHEREMAPRENDERER_H
#define SPHEREMAPRENDERER_H

#include <LLVK_GeometryLoaderV2.hpp>
#include "SphereMapPass.hpp"
#include "ScenePass.hpp"
#include "VulkanRenderer.h"

LLVK_NAMESPACE_BEGIN
namespace SPHEREMAP_NAMESPACE {
class SphereMapRenderer :  public VulkanRenderer{
public:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    void recordCommandBuffer();
    VkDescriptorPool descPool{};
    SphereMapPass<SphereMapRenderer, GLTFLoaderV2::Loader<VTXFmt_P>> sphereMapPass{};
    ScenePass<SphereMapRenderer, GLTFLoaderV2::Loader<VTXFmt_P_N>> scenePass{};
};
}

LLVK_NAMESPACE_END


#endif //SPHEREMAPRENDERER_H
