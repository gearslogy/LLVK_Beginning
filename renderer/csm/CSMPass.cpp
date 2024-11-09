//
// Created by liuya on 11/8/2024.
//

#include "CSMPass.h"
#include "LLVK_Image.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "VulkanRenderer.h"
LLVK_NAMESPACE_BEGIN
void CSMPass::prepare() {

}

void CSMPass::cleanup() {
    UT_Fn::cleanup_resources(depthAttachment);
}

void CSMPass::prepareDepthResources() {
    auto device = pRenderer->getMainDevice().logicalDevice;
    depthSampler = FnImage::createDepthSampler(device);
    setRequiredObjectsByRenderer(pRenderer, depthAttachment);
    depthAttachment.createDepth32(width,width, depthSampler);

}

LLVK_NAMESPACE_END
