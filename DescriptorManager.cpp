//
// Created by star on 5/12/2024.
//

#include "DescriptorManager.h"
#include "Utils.h"
#include <array>
#include <chrono>
#include <iostream>
std::array<VkDescriptorSetLayoutBinding, 2> LayoutBindings::getDescriptorSetLayoutBindings(VkDevice device) {
    VkDescriptorSetLayoutBinding binding01{};
    VkDescriptorSetLayoutBinding binding02{};
    binding01.binding = 0;
    binding01.descriptorCount = 1; // descriptorCount specifies the number of values in the array
    binding01.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding01.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    binding01.pImmutableSamplers = nullptr;

    binding02.binding = 1;
    binding02.descriptorCount = 1; // descriptorCount specifies the number of values in the array
    binding02.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding02.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    binding02.pImmutableSamplers = nullptr;
    const std::array bindings = {binding01, binding02};
    return bindings;
}
VkDescriptorSetLayout LayoutBindings::createDescriptorSetLayout(VkDevice device) {
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 2;
    createInfo.pBindings = getDescriptorSetLayoutBindings(device).data();

    if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    return descriptorSetLayout;
}




void UniformBuffers_FrameFlighted::createUniformBuffers() {
    uboObject1.resize(MAX_FRAMES_IN_FLIGHT);
    uboObject2.resize(MAX_FRAMES_IN_FLIGHT);
    for (auto &[buf,mem, mapped]: uboObject1) {
        FnBuffer::createBuffer(bindPhysicalDevice, bindDevice, sizeof(UBO1),
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buf, mem);
        vkMapMemory(bindDevice, mem, 0, sizeof(UBO1), 0, &mapped);
    }

    for (auto &[buf,mem,mapped]: uboObject2) {
        FnBuffer::createBuffer(bindPhysicalDevice, bindDevice, sizeof(UBO2),
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buf, mem);
        vkMapMemory(bindDevice, mem, 0, sizeof(UBO2), 0, &mapped);
    }
}

void UniformBuffers_FrameFlighted::cleanup() {
    for (auto &bm: uboObject1)
        bm.cleanup(bindDevice);
    for (auto &bm: uboObject2)
        bm.cleanup(bindDevice);
    uboObject1.clear();
    uboObject2.clear();
}

void UniformBuffers_FrameFlighted::updateUniform(uint32_t currentFrame) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


    UBO1 ubo1{};
    ubo1.dynamicsColor = { time = std::sin(time),1,1};
    ubo1.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //ubo1.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo1.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));// OGL
    ubo1.proj = glm::perspective(glm::radians(45.0f), bindSwapChainExtent->width / (float) bindSwapChainExtent->height, 0.1f, 10.0f);
    ubo1.proj[1][1] *= -1;


    UBO2 ubo2{};
    ubo2.baseAmp = std::sin(time);
    ubo2.specularAmp = std::cos(time);
    std::cout << "time:"<< time << "      "  <<ubo2.specularAmp<< std::endl;
    ubo2.base = {1,0,0,1};
    ubo2.specular = {0,1,0,1};
    ubo2.normal = {0,0,1,1};
    ubo2.aa= {1,1,0};
    ubo2.bb= {1,1,1,1};


    auto &[buf1, mem1, mapped1] = uboObject1[currentFrame];
    memcpy(mapped1, &ubo1, sizeof(UBO1));

    auto &[buf2, mem2, mapped2] = uboObject2[currentFrame];
    memcpy(mapped2, &ubo2, sizeof(UBO2));
}


void DescriptorManager::createDescriptorSetLayout() {
    descriptorSetLayout = LayoutBindings::createDescriptorSetLayout(bindDevice);
}
void DescriptorManager::createUniformBuffers() {
    simpleUniformBuffer.bindDevice = bindDevice;
    simpleUniformBuffer.bindPhysicalDevice = bindPhyiscalDevice;
    simpleUniformBuffer.bindSwapChainExtent = bindSwapChainExtent;
    simpleUniformBuffer.createUniformBuffers();
}


void DescriptorManager::createDescriptorPool() {
    /*
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT}, // 2个Uniform Buffer
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} // 1个Combined Image Sampler
    };*/

    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, num_ubos * MAX_FRAMES_IN_FLIGHT }
    };
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;; // 目前一个pool，pool是VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,里面每帧2个UBO
    createInfo.pPoolSizes = poolSizes;
    createInfo.maxSets =  static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);// 我们最大创建MAX_FRAMES_IN_FLIGHT(2)个set，一帧一个
    if(vkCreateDescriptorPool(bindDevice, &createInfo, nullptr, &descriptorPool)!=VK_SUCCESS) {
        throw std::runtime_error{"ERROR create descriptor pool"};
    }

}

void DescriptorManager::createDescriptorSets() {
    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT); // 每帧一个set. 所以材质里uniform buffer: layout(set = 0, binding = 0) uniform UniformBufferObject { ... } set只能是0

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout; // 每个set中的layout都是一样的

    //每帧只有一个set=0，然后layout中是有2个 uniform buffer. 一个binding=0 一个binding=1
    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        if(vkAllocateDescriptorSets(bindDevice, &allocInfo, &descriptorSets[i]) != VK_SUCCESS) {
            throw std::runtime_error{"can not create descriptor set"};
        }
    }

    // same as below code!下面是一次性创建。
    /*
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    */

    // 关联

    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        VkDescriptorBufferInfo uboBufferInfo1{};
        uboBufferInfo1.buffer = simpleUniformBuffer.uboObject1[i].buffer;
        uboBufferInfo1.offset = 0;
        uboBufferInfo1.range = sizeof(UBO1);

        VkDescriptorBufferInfo uboBufferInfo2{};
        uboBufferInfo2.buffer = simpleUniformBuffer.uboObject2[i].buffer;
        uboBufferInfo2.offset = 0;
        uboBufferInfo2.range = sizeof(UBO2);

        VkWriteDescriptorSet descriptorWrite1{};
        descriptorWrite1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite1.dstSet = descriptorSets[i];
        descriptorWrite1.dstBinding = 0;
        descriptorWrite1.dstArrayElement = 0;// NOT ARRAY UBO
        descriptorWrite1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite1.descriptorCount = 1;// NOT ARRAY UBO
        descriptorWrite1.pBufferInfo = &uboBufferInfo1;

        VkWriteDescriptorSet descriptorWrite2{};
        descriptorWrite2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite2.dstSet = descriptorSets[i];
        descriptorWrite2.dstBinding = 1;
        descriptorWrite2.dstArrayElement = 0;// NOT ARRAY UBO
        descriptorWrite2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite2.descriptorCount = 1; // NOT ARRAY UBO
        descriptorWrite2.pBufferInfo = &uboBufferInfo2;

        std::array descriptorWrites = {descriptorWrite1,descriptorWrite2};
        vkUpdateDescriptorSets(bindDevice, 2, descriptorWrites.data(), 0, nullptr);

    }



}

void DescriptorManager::cleanup() {
    simpleUniformBuffer.cleanup();
    vkDestroyDescriptorPool(bindDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(bindDevice, descriptorSetLayout, nullptr);
}
