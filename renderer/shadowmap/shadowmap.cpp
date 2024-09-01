//
// Created by liuya on 9/1/2024.
//

#include "shadowmap.h"
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
LLVK_NAMESPACE_BEGIN
void shadowmap::render() {
}

void shadowmap::cleanupObjects() {
    auto device = mainDevice.logicalDevice;
    vkDestroySampler(device, sampler, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    geoBufferManager.cleanup();
    foliageTex.cleanup();
    gridTex.cleanup();
}

void shadowmap::prepare() {
    loadTextures();
    loadModels();
}

void shadowmap::loadTextures() {
    sampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    setRequiredObjects(foliageTex, gridTex);
    foliageTex.create("content/plants/gardenplants/tex_array.ktx2", sampler );
    gridTex.create("content/shadowmap/grid_tex/tex_array.ktx2",sampler);
}

void shadowmap::loadModels() {
    setRequiredObjects(geoBufferManager);
    gridGeo.load("content/shadowmap/grid.gltf");
    foliageGeo.load("content/plants/gardenplants/var0.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(gridGeo, geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(foliageGeo, geoBufferManager);
}


void shadowmap::prepareOffscreen() {

}

void shadowmap::prepareUniformBuffers() {
}

void shadowmap::prepareDescriptorSets() {
    const auto &device = mainDevice.logicalDevice;
    const auto &physicalDevice = mainDevice.physicalDevice;
    // cal the UBO count
    constexpr auto ubo_depth_map_gen_count = 2; // depth mvp opacity and opaque
    constexpr auto tex_depth_map_gen_opacity_count = 1; // sample opacity map to discard.
    constexpr auto tex_depth_map_gen_opaque_count = 0;  // opaque object do not need any sampler2d or 2d array

    constexpr auto scene_ubo_count = 2; //  opacity and opaque
    constexpr auto scene_opacity_tex_sampler2d_count = 2; // texture + depth
    constexpr auto scene_opaque_tex_sampler2d_count = 2;  // texture + depth

    constexpr auto UBO_COUNT = ubo_depth_map_gen_count + scene_ubo_count;
    constexpr auto TEX_COUNT = tex_depth_map_gen_opacity_count +
                               tex_depth_map_gen_opaque_count +
                               scene_opacity_tex_sampler2d_count +
                               scene_opaque_tex_sampler2d_count;

    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UBO_COUNT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TEX_COUNT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 3 * 2); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert(result == VK_SUCCESS);

    // we only have one set.
    auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);         // ubo
    auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT); // maps
    auto set0_ubo_binding2 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_VERTEX_BIT); // depth
    const std::array set0_bindings = {set0_ubo_binding0, set0_ubo_binding1, set0_ubo_binding2};

    const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(set0_bindings);
    if(vkCreateDescriptorSetLayout(device, &setLayoutCIO, nullptr, &descriptorSets.descriptorSetLayout)!=VK_SUCCESS)
        throw std::runtime_error{"Error create plant ubo set layout"};


}

void shadowmap::preparePipelines() {

}

void shadowmap::updateUniformBuffers() {

}


LLVK_NAMESPACE_END
