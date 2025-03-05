//
// Created by liuyangping on 2025/2/26.
//

#include "SubPassRenderer.h"
#include <memory>

#include "SubPassResource.h"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
void SubPassRenderer::prepare() {
    usedDevice = getMainDevice().logicalDevice;
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
    setRequiredObjectsByRenderer(this, attachments.albedo, attachments.NRM);

    if(attachments.albedo.isValid)
        attachments.albedo.cleanup();
    attachments.albedo.create(w,h , VK_FORMAT_R8G8B8A8_UNORM, colorSampler, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

    if(attachments.NRM.isValid)
        attachments.NRM.cleanup();
    attachments.NRM.create(w,h , VK_FORMAT_R8G8B8A8_UNORM, colorSampler, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

}

void SubPassRenderer::createFramebuffers() {

}


LLVK_NAMESPACE_END