//
// Created by liuya on 11/11/2024.
//

#include "GeometryContainers.h"
#include "LLVK_Descriptor.hpp"
#include "VulkanRenderer.h"
LLVK_NAMESPACE_BEGIN



void RenderContainerTwoSet::buildSet() {
    const auto &device = requiredObjects.pRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const auto &ubo_setLayout = *requiredObjects.pSetLayoutUBO;
    const auto &tex_setLayout = *requiredObjects.pSetLayoutTexture;

    const std::vector ubo_layouts(MAX_FRAMES_IN_FLIGHT, ubo_setLayout);
    const std::vector tex_layouts(MAX_FRAMES_IN_FLIGHT, tex_setLayout);
    auto uboSetAllocInfo = FnDescriptor::setAllocateInfo(pool,ubo_layouts);
    auto texSetAllocInfo = FnDescriptor::setAllocateInfo(pool,tex_layouts);
    for(auto &geo : renderDelegates) {
        UT_Fn::invoke_and_check("Error create RenderContainerTwoSet::uboSets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,geo.setUBOs.data());
        UT_Fn::invoke_and_check("Error create RenderContainerTwoSet::texSets",vkAllocateDescriptorSets,device, &texSetAllocInfo,geo.setTextures.data());
    }
    for(int frame=0; frame<MAX_FRAMES_IN_FLIGHT; frame++) {
        const auto &uboBuffer = *requiredObjects.pUBOs[frame];
        for(const auto &geo: renderDelegates) {
            const auto &ubo_set = geo.setUBOs[frame];
            const auto &tex_set = geo.setTextures[frame];
            // UBO
            std::vector<VkWriteDescriptorSet> writeSets;
            writeSets.emplace_back(FnDescriptor::writeDescriptorSet(ubo_set,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uboBuffer.descBufferInfo));
            // tex
            for(auto &&[idx, tex] : UT_Fn::enumerate(geo.pTextures))
                writeSets.emplace_back(FnDescriptor::writeDescriptorSet(tex_set,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,idx, &tex->descImageInfo)); // set1
            vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
        }
    }
}
void RenderContainerTwoSet::draw(const VkCommandBuffer &cmdBuf, const VkPipelineLayout &pipelineLayout) {
    VkDeviceSize offsets[1] = { 0 };
    const auto currentFrame = requiredObjects.pRenderer->getCurrentFrame();
    for(const auto &geo : renderDelegates) {
        const GLTFLoader::Part *gltfPartGeo = geo.pGeometry;
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &gltfPartGeo->verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,gltfPartGeo->indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        std::array bindSets = {geo.setUBOs[currentFrame], geo.setTextures[currentFrame]};
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, bindSets.size(), bindSets.data(), 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, gltfPartGeo->indices.size(), 1, 0, 0, 0);
    }
}




void RenderContainerOneSet::buildSet() {
    const auto &device = requiredObjects.pRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const auto &setLayout = *requiredObjects.pSetLayout;
    const std::vector setLayouts(MAX_FRAMES_IN_FLIGHT, setLayout);
    auto setAllocInfo = FnDescriptor::setAllocateInfo(pool,setLayouts);
    for(auto &geo : renderDelegates) {
        UT_Fn::invoke_and_check("Error create RenderContainerOneSet::uboSets", vkAllocateDescriptorSets, device, &setAllocInfo, geo.descSets.data());
    }
    for(int frame=0;frame<MAX_FRAMES_IN_FLIGHT;frame++) {
        const auto &uboBuffer = *requiredObjects.pUBOs[frame];
        for(const auto &geo: renderDelegates) {
            const auto &descSet = geo.descSets[frame];
            // UBO
            std::vector<VkWriteDescriptorSet> writeSets;
            writeSets.emplace_back(FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uboBuffer.descBufferInfo));
            // tex
            for(auto &&[idx, tex] : UT_Fn::enumerate(geo.pTextures))
                writeSets.emplace_back(FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,idx + 1, &tex->descImageInfo));
            vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
        }
    }
}

void RenderContainerOneSet::draw(const VkCommandBuffer &cmdBuf, const VkPipelineLayout &pipelineLayout) {
    VkDeviceSize offsets[1] = { 0 };
    for(const auto &geo : renderDelegates) {
        const auto &descSet = geo.descSets[requiredObjects.pRenderer->getCurrentFrame()];
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr);
        UT_GeometryContainer::renderPart(cmdBuf,  geo.pGeometry);
    }
}


// shared set functions
void RenderContainerOneSharedSet::buildSets(const VkDescriptorSetLayout *pSetLayout) {
    const auto &device = requiredObjects.pRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const std::vector setLayouts(MAX_FRAMES_IN_FLIGHT, *pSetLayout);
    auto setAllocInfo = FnDescriptor::setAllocateInfo(pool,setLayouts);
    UT_Fn::invoke_and_check("Error create RenderContainerOneSet::uboSets", vkAllocateDescriptorSets, device, &setAllocInfo, descSets.data());
    // update sets
    for(int frame=0;frame<MAX_FRAMES_IN_FLIGHT;frame++) {
        const auto &uboBuffer = *pUBOs[frame];
        const auto &descSet = descSets[frame];
        // UBO
        std::vector<VkWriteDescriptorSet> writeSets;
        writeSets.emplace_back(FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uboBuffer.descBufferInfo));
        // tex
        for(auto &&[idx, tex] : UT_Fn::enumerate(pTextures))
            writeSets.emplace_back(FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,idx + 1, &tex->descImageInfo));
        vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
    }
}
void RenderContainerOneSharedSet::draw(const VkCommandBuffer &cmdBuf, const VkPipelineLayout &pipelineLayout) {
    VkDeviceSize offsets[1] = { 0 };
    const auto &descSet = descSets[requiredObjects.pRenderer->getCurrentFrame()];
    vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    for(const auto *partGeo : renderGeos)
        UT_GeometryContainer::renderPart(cmdBuf, partGeo);
}





LLVK_NAMESPACE_END