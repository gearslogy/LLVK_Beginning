//
// Created by liuyangping on 2025/2/26.
//

#include "SubPassRenderer.h"
#include <memory>

#include "SubPassResource.h"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
void SubPassRenderer::prepare() {
    2 = getMainDevice().logicalDevice;
    usedPhyDevice = getMainDevice().physicalDevice;

    resourceLoader = std::make_unique<SubPassResource>();
    resourceLoader->renderer = this;
    resourceLoader->prepare();
    colorSampler = FnImage::createImageSampler(usedPhyDevice, usedDevice);
}
void SubPassRenderer::cleanupObjects() {
    resourceLoader->cleanup();
    UT_Fn::cleanup_sampler(usedDevice, colorSampler);
}

void SubPassRenderer::render() {

}

void SubPassRenderer::createAttachments() {
    auto w = simpleSwapchain.swapChainExtent.width;
    auto h = simpleSwapchain.swapChainExtent.height;
    VkImageUsageFlagBits colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    setRequiredObjectsByRenderer(this, attachments.Albedo, attachments.NOM);
    attachments.Albedo.create(w,h , VK_FORMAT_R8G8B8A8_UNORM, attachmentSampler.)
}
void SubPassRenderer::swapChainResize() {

}



LLVK_NAMESPACE_END