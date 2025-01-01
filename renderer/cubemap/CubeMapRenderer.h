//
// Created by liuya on 12/23/2024.
//

#ifndef CUBEMAP_H
#define CUBEMAP_H
#include <LLVK_GeometryLoaderV2.hpp>
#include "CubeMapPass.hpp"
#include "ScenePass.hpp"
#include "VulkanRenderer.h"

LLVK_NAMESPACE_BEGIN


namespace CUBEMAP_NAMESPACE {
class CubeMapRenderer : public VulkanRenderer{
public:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    void recordCommandBuffer();
    VkDescriptorPool descPool;
    CubeMapPass<CubeMapRenderer, GLTFLoaderV2::Loader<VTXFmt_P>> cubeMapPass;
    ScenePass<CubeMapRenderer, GLTFLoaderV2::Loader<VTXFmt_P_N>> scenePass;
};
}
LLVK_NAMESPACE_END

#endif //CUBEMAP_H
