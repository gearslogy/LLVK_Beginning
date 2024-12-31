//
// Created by liuya on 12/23/2024.
//

#ifndef CUBEMAP_H
#define CUBEMAP_H
#include <LLVK_GeometryLoaderV2.hpp>
#include "CubeMapPass.hpp"
#include "VulkanRenderer.h"

LLVK_NAMESPACE_BEGIN



class CubeMapRenderer : public VulkanRenderer{
public:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    void recordCommandBuffer();
    VkDescriptorPool descPool;
    CubeMapPass<CubeMapRenderer, GLTFLoaderV2::Loader<VTXFmt_P>> cubeMapPass;
};

LLVK_NAMESPACE_END

#endif //CUBEMAP_H
