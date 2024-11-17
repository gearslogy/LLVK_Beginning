//
// Created by liuya on 11/11/2024.
//

#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_VmaBuffer.hpp>
#include "DualpassRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"

LLVK_NAMESPACE_BEGIN
void DualPassRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_resources(geometryManager, tex);
    UT_Fn::cleanup_sampler(mainDevice.logicalDevice, colorSampler);
    vkDestroyDescriptorPool(device, descPool, VK_NULL_HANDLE);
    UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);
}


void DualPassRenderer::prepare() {
    // 1. pool
    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert(result == VK_SUCCESS);
    // 2. UBO create
    setRequiredObjectsByRenderer(this, uboBuffers[0], uboBuffers[1]);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uboData));

    // 3 descSetLayout
    const std::array setLayoutBindings = {
        FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT),
        FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
    };
    const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(setLayoutBindings);
    UT_Fn::invoke_and_check("Error create desc set layout",vkCreateDescriptorSetLayout,device, &setLayoutCIO, nullptr, &descSetLayout);

    // 4. model resource loading
    colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    setRequiredObjectsByRenderer(this, geometryManager);
    headLoader.load("content/scene/dualpass/head.gltf");
    hairLoader.load("content/scene/dualpass/hair.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(headLoader, geometryManager); // GPU buffer allocation
    UT_VmaBuffer::addGeometryToSimpleBufferManager(hairLoader, geometryManager); // GPU buffer allocation
    tex.create("content/scene/dualpass/gpu_D.ktx2", colorSampler);

    const std::array setLayouts({descSetLayout,descSetLayout});
    auto setAllocInfo = FnDescriptor::setAllocateInfo(descPool,setLayouts);
    UT_Fn::invoke_and_check("Error create RenderContainerOneSet::uboSets", vkAllocateDescriptorSets, device, &setAllocInfo, descSets.data());
    // pipeline

}

void DualPassRenderer::render() {

}


LLVK_NAMESPACE_END