//
// Created by liuya on 12/8/2024.
//

#include "RbdVatRenderer.h"

#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
RbdVatRenderer::RbdVatRenderer()  = default;

void RbdVatRenderer::prepare() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    setRequiredObjectsByRenderer(this, geomManager);
    setRequiredObjectsByRenderer(this, texPosition,texRotation );
    buildings.load("content/scene/rbd_vat/gltf/buildings.gltf");
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    vatSampler = FnImage::createExrVATSampler(device);
    texPosition.create("content/scene/vat_tex/position.ktx2",vatSampler);
    texRotation.create("content/scene/vat_tex/orient.ktx2",vatSampler);
}

void RbdVatRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    UT_Fn::cleanup_resources(geomManager);
    UT_Fn::cleanup_sampler(device, colorSampler, vatSampler);
}




LLVK_NAMESPACE_END