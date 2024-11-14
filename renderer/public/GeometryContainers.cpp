//
// Created by liuya on 11/11/2024.
//

#include "GeometryContainers.h"
#include "../../LLVK_Descriptor.hpp"
#include "../../VulkanRenderer.h"
LLVK_NAMESPACE_BEGIN


template<typename T>
inline void updateWriteSets(const auto &requiredObjects , const std::vector<T> &renderDelegates, uint32_t texBindingIdOffset) {
    const auto &device = requiredObjects.pVulkanRenderer->getMainDevice().logicalDevice;
    for(int frame=0;frame<MAX_FRAMES_IN_FLIGHT;frame++) {
        const auto &uboBuffer = *requiredObjects.pUBOs[frame];
        for(const auto &geo: renderDelegates) {
            const auto &ubo_set = geo.setUBOs[frame];
            const auto &tex_set = geo.setTextures[frame];
            // UBO
            std::vector<VkWriteDescriptorSet> writeSets;
            writeSets.emplace_back(FnDescriptor::writeDescriptorSet(ubo_set,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uboBuffer.descBufferInfo));
            // tex
            for(auto &&[idx, tex] : UT_Fn::enumerate(geo.pTextures))
                writeSets.emplace_back(FnDescriptor::writeDescriptorSet(tex_set,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,idx + texBindingIdOffset, &tex->descImageInfo));
            vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
        }
    }
}




void RenderContainerTwoSet::buildSet() {
    const auto &device = requiredObjects.pVulkanRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const auto &ubo_setLayout = *requiredObjects.pSetLayoutUBO;
    const auto &tex_setLayout = *requiredObjects.pSetLayoutTexture;

    const std::vector ubo_layouts(MAX_FRAMES_IN_FLIGHT, ubo_setLayout);
    const std::vector tex_layouts(MAX_FRAMES_IN_FLIGHT, tex_setLayout);
    auto uboSetAllocInfo = FnDescriptor::setAllocateInfo(pool,ubo_layouts);
    auto texSetAllocInfo = FnDescriptor::setAllocateInfo(pool,tex_layouts);
    auto allocateSetForObjects = [&](std::vector<RenderDelegate> &objs) {
        for(auto &geo : objs) {
            UT_Fn::invoke_and_check("Error create RenderContainerTwoSet::uboSets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,geo.setUBOs.data());
            UT_Fn::invoke_and_check("Error create RenderContainerTwoSet::texSets",vkAllocateDescriptorSets,device, &texSetAllocInfo,geo.setTextures.data());
        }
    };
    allocateSetForObjects(renderDelegates);
    updateWriteSets(requiredObjects, renderDelegates, 0U);

}


void RenderContainerOneSet::buildSet() {
    const auto &device = requiredObjects.pVulkanRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const auto &setLayout = *requiredObjects.pSetLayout;
    const std::vector setLayouts(MAX_FRAMES_IN_FLIGHT, setLayout);
    auto setAllocInfo = FnDescriptor::setAllocateInfo(pool,setLayouts);
    for(auto &geo : renderDelegates) {
        UT_Fn::invoke_and_check("Error create RenderContainerOneSet::uboSets", vkAllocateDescriptorSets, device, &setAllocInfo, geo.setUBOs.data());
        UT_Fn::invoke_and_check("Error create RenderContainerOneSet::texSets", vkAllocateDescriptorSets, device, &setAllocInfo, geo.setTextures.data());
    }
    updateWriteSets(requiredObjects, renderDelegates, 1U);
}
void RenderContainerOneSet::cmdBindDescriptorSets() {
    requiredObjects.pVulkanRenderer->getCurrentFrame();
}





LLVK_NAMESPACE_END