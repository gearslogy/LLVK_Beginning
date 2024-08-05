//
// Created by star on 5/12/2024.
//

#include "DescriptorManager.h"
#include "Utils.h"
#include <array>
#include <chrono>
#include <iostream>
LLVK_NAMESPACE_BEGIN
std::array<VkDescriptorSetLayoutBinding, 2> LayoutBindings::getUBODescriptorSetLayoutBindings(VkDevice device) {
    // 有2个UBO 物体
    VkDescriptorSetLayoutBinding binding01{};
    VkDescriptorSetLayoutBinding binding02{};
    binding01.binding = 0;
    binding01.descriptorCount = 1; // descriptorCount specifies the number of values in the array
    binding01.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding01.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    binding01.pImmutableSamplers = nullptr;

    binding02.binding = 1;
    binding02.descriptorCount = 1; // descriptorCount specifies the number of values in the array
    binding02.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding02.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    binding02.pImmutableSamplers = nullptr;
    const std::array bindings = {binding01, binding02};
    return bindings;
}
std::array<VkDescriptorSetLayoutBinding, 1> LayoutBindings::getTextureDescriptorSetLayoutBindings(VkDevice device) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0; // START FROM 0
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    const std::array bindings = {samplerLayoutBinding};
    return bindings;
}

VkDescriptorSetLayout LayoutBindings::createUBODescriptorSetLayout(VkDevice device) {
    auto uboLayoutBindings = getUBODescriptorSetLayoutBindings(device);
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = uboLayoutBindings.size();
    createInfo.pBindings = uboLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    return descriptorSetLayout;
}

VkDescriptorSetLayout LayoutBindings::createTextureDescriptorSetLayout(VkDevice device) {
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = getTextureDescriptorSetLayoutBindings(device).size();
    createInfo.pBindings = getTextureDescriptorSetLayoutBindings(device).data();

    if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    return descriptorSetLayout;
}

// -----------------------------Simple mangaer functions-------------------------------------------------
void DescriptorManager::createTexture(const char *tex) {
    assert(bindPhysicalDevice!=VK_NULL_HANDLE);
    assert(bindDevice!=VK_NULL_HANDLE);
    assert(bindCommandPool!=VK_NULL_HANDLE);
    assert(bindQueue!=VK_NULL_HANDLE);
    imageAndMemory= FnImage::createTexture(bindPhysicalDevice, bindDevice, bindCommandPool, bindQueue,
          tex
      );
    FnImage::createImageView(bindDevice, imageAndMemory.image,
        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, imageAndMemory.mipLevels, imageView);

    imageSampler = FnImage::createImageSampler(bindPhysicalDevice, bindDevice);
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
    ubo1.screenSize = {bindSwapChainExtent->width, bindSwapChainExtent->height};
    ubo1.model = 1.0f;
    //ubo1.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //ubo1.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo1.view = glm::lookAt(glm::vec3(0, 80.0f, 200), glm::vec3(0.0f, 52, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));// OGL
    ubo1.proj = glm::perspective(glm::radians(45.0f), bindSwapChainExtent->width / (float) bindSwapChainExtent->height, 0.1f, 2000.0f);
    ubo1.proj[1][1] *= -1;


    UBO2 ubo2{};
    ubo2.baseAmp = std::sin(time);
    ubo2.specularAmp = std::cos(time);
    //std::cout << "time:"<< time << "      "  <<ubo2.specularAmp<< std::endl;
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
    ubo_descriptorSetLayout = LayoutBindings::createUBODescriptorSetLayout(bindDevice);
    texture_descriptorSetLayout = LayoutBindings::createTextureDescriptorSetLayout(bindDevice);
}
void DescriptorManager::createUniformBuffers() {
    simpleUniformBuffer.bindDevice = bindDevice;
    simpleUniformBuffer.bindPhysicalDevice = bindPhysicalDevice;
    simpleUniformBuffer.bindSwapChainExtent = bindSwapChainExtent;
    simpleUniformBuffer.createUniformBuffers();
}


void DescriptorManager::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes= {{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, num_ubos * MAX_FRAMES_IN_FLIGHT },
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT} // 2 个Combined Image Sampler
    }};

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = poolSizes.size() ;
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = numCreatedSets;//一帧2个set(set=0服务UBO，set=1服务Texture)  有2帧，所以有4个。一帧操作2个
    if(vkCreateDescriptorPool(bindDevice, &createInfo, nullptr, &descriptorPool)!=VK_SUCCESS) {
        throw std::runtime_error{"ERROR create descriptor pool"};
    }

}

void DescriptorManager::createDescriptorSets() {
    descriptorSets.resize(numCreatedSets);
    std::vector<VkDescriptorSetLayout> layouts(numCreatedSets);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        layouts[i * 2] = ubo_descriptorSetLayout;
        layouts[i * 2 + 1] = texture_descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();
    if(vkAllocateDescriptorSets(bindDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error{"can not create descriptor set"};
    }

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
        descriptorWrite1.dstSet = descriptorSets[i*2];
        descriptorWrite1.dstBinding = 0;                            // layout(set=0, binding =0) uniform UniformBufferObject {}
        descriptorWrite1.dstArrayElement = 0;// NOT ARRAY UBO
        descriptorWrite1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite1.descriptorCount = 1;// NOT ARRAY UBO
        descriptorWrite1.pBufferInfo = &uboBufferInfo1;

        VkWriteDescriptorSet descriptorWrite2{};
        descriptorWrite2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite2.dstSet = descriptorSets[i*2];
        descriptorWrite2.dstBinding = 1;                               //layout(set=0, binding =1) uniform UniformBufferObject {}
        descriptorWrite2.dstArrayElement = 0;// NOT ARRAY UBO
        descriptorWrite2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite2.descriptorCount = 1; // NOT ARRAY UBO
        descriptorWrite2.pBufferInfo = &uboBufferInfo2;

        // image
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = imageSampler;

        VkWriteDescriptorSet textureWrite{};
        textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite.dstSet = descriptorSets[i*2 + 1];
        textureWrite.dstBinding = 0;     // 注意从0 开始绑定 贴图， layout(set=1, binding = 0) uniform sampler2D texSampler;
        textureWrite.dstArrayElement = 0;// NOT ARRAY UBO
        textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrite.descriptorCount = 1; // NOT ARRAY UBO
        textureWrite.pImageInfo = &imageInfo;


        std::array descriptorWrites = {descriptorWrite1,descriptorWrite2, textureWrite};
        vkUpdateDescriptorSets(bindDevice,
            descriptorWrites.size(),
            descriptorWrites.data(),
            0,
            nullptr);

    }
}

void DescriptorManager::cleanup() {
    simpleUniformBuffer.cleanup();
    vkDestroyDescriptorPool(bindDevice, descriptorPool, nullptr);
    // free image
    vkDestroyImage(bindDevice, imageAndMemory.image, nullptr);
    vkDestroySampler(bindDevice,imageSampler,nullptr);
    vkDestroyImageView(bindDevice,imageView,nullptr);
    vkFreeMemory(bindDevice, imageAndMemory.memory, nullptr);

    vkDestroyDescriptorSetLayout(bindDevice, ubo_descriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(bindDevice, texture_descriptorSetLayout, nullptr);
}
LLVK_NAMESPACE_END