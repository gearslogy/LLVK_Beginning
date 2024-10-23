//
// Created by lp on 2024/9/19.
//

#include "JsonSceneParser.h"
#include <fstream>
#include "LLVK_UT_Json.hpp"
#include "LLVK_Descriptor.hpp"
#include "VulkanRenderer.h"
#include "renderer/shadowmap_v2/UT_ShadowMap.hpp"
LLVK_NAMESPACE_BEGIN
void InstanceGeometryContainer::buildSet() {
    const auto &device = requiredObjects.pVulkanRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const auto &ubo_setLayout = *requiredObjects.pSetLayoutUBO;
    const auto &tex_setLayout = *requiredObjects.pSetLayoutTexture;
    const auto &ubo = *requiredObjects.pUBO;


    auto uboSetAllocInfo = FnDescriptor::setAllocateInfo(pool,&ubo_setLayout, 1);
    auto texSetAllocInfo = FnDescriptor::setAllocateInfo(pool,&tex_setLayout, 1);
    auto allocateSetForObjects = [&](auto &objs) {
        for(auto &geo : objs) {
            UT_Fn::invoke_and_check("Error create terrain ubo sets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,&geo.setUBO);
            UT_Fn::invoke_and_check("Error create terrain tex sets",vkAllocateDescriptorSets,device, &texSetAllocInfo,&geo.setTexture);
        }
    };
    allocateSetForObjects(opaqueRenderableObjects);

    auto updateWriteSets = [&,this](auto &&objs) {
        for(const auto &geo: objs) {
            const auto &ubo_set = geo.setUBO;
            const auto &tex_set = geo.setTexture;
            // UBO
            std::vector<VkWriteDescriptorSet> writeSets;
            writeSets.emplace_back(FnDescriptor::writeDescriptorSet(ubo_set,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &ubo.descBufferInfo));
            // tex
            for(auto &&[idx, tex] : UT_Fn::enumerate(geo.pTextures))
                writeSets.emplace_back(FnDescriptor::writeDescriptorSet(tex_set,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,idx, &tex->descImageInfo));
            vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
        }
    };
    updateWriteSets(opaqueRenderableObjects);

}

JsonPointsParser::JsonPointsParser(const std::string &path){
    std::ifstream in(path.data());
    if (!in.good())
        throw std::runtime_error{std::string{"Could not open file "} + path + "."};
    in >> jsHandle;
    if(not jsHandle.contains("points") )
        throw std::runtime_error{std::string{"Could not parse JSON file, no points key "} };
    const nlohmann::json &points = jsHandle[points];

    for (auto &data: points) {
        const auto &P = data["P"].get<glm::vec3>();
        const auto &orient = data["orient"].get<glm::vec4>();
        const auto &pscale = data["scale"].get<float>();
        instanceData.emplace_back(InstanceData{P, orient, pscale});
    }
    std::cout << "[[JsonSceneParser]]" << " npts:" << instanceData.size() << std::endl;
}



InstancedObjectPass::InstancedObjectPass(const VulkanRenderer *renderer, const VkDescriptorPool *descPool):pRenderer(renderer),pDescriptorPool(descPool) { }

void InstancedObjectPass::cleanup() {
    instanceBufferManager.cleanup();
}

void InstancedObjectPass::prepareInstanceData() {
    setRequiredObjectsByRenderer(pRenderer,instanceBufferManager);
    JsonPointsParser parser{"content/scene/instance/scene_json/trees.json"};
    VkDeviceSize bufferSize = sizeof(InstanceData) * std::size(parser.instanceData);
    instanceBufferManager.createBufferWithStagingBuffer<VK_BUFFER_USAGE_VERTEX_BUFFER_BIT>(bufferSize, parser.instanceData.data());
}




LLVK_NAMESPACE_END