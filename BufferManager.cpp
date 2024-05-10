//
// Created by star on 5/7/2024.
//

#include "BufferManager.h"
#include <stdexcept>
#include "Utils.h"
#include "libs/VulkanMemoryAllocator-3.0.1/include/vk_mem_alloc.h"

void BufferManager::cleanup() {
    for(auto [buffer, bufferMemory] : createdBuffers) {
        vkDestroyBuffer(bindDevice, buffer, nullptr);
        vkFreeMemory(bindDevice,bufferMemory, nullptr);
    }
    createdBuffers.clear();

    for(auto [buffer, bufferMemory] : createdIndexedBuffers) {
        vkDestroyBuffer(bindDevice, buffer, nullptr);
        vkFreeMemory(bindDevice, bufferMemory, nullptr);
    }
    createdIndexedBuffers.clear();
}


void BufferManager::createBuffer(VkDeviceSize size,
                                 VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags memProperties,
                                 VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if(vkCreateBuffer(bindDevice, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error{"can not create buffer"};
    }
    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(bindDevice, buffer, &memReqs);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memReqs.size;
    allocateInfo.memoryTypeIndex = findMemoryType(bindPhysicalDevice, memReqs.memoryTypeBits, memProperties);
    vkAllocateMemory(bindDevice, &allocateInfo, nullptr, &bufferMemory);
    vkBindBufferMemory(bindDevice, buffer, bufferMemory,0);// bind buffer <<--->> buffer memory
}

void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // ---------- 1. RECORD--------------
    // create copy command
    VkCommandBufferAllocateInfo cmdBufAllocInfo{};
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocInfo.commandPool = bindCommandPool;
    cmdBufAllocInfo.commandBufferCount = 1;
    VkCommandBuffer copyCommandBuffer{};
    vkAllocateCommandBuffers(bindDevice, &cmdBufAllocInfo, &copyCommandBuffer);

    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // only once

    if(vkBeginCommandBuffer(copyCommandBuffer, &cmdBufBeginInfo) != VK_SUCCESS) throw std::runtime_error{"begin copy command buffer error"};
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(copyCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    if(vkEndCommandBuffer(copyCommandBuffer) != VK_SUCCESS)
        throw std::runtime_error{"begin copy command buffer error"};
    // ------------- 2. SUBMIT-------------------
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    vkQueueSubmit(bindQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkDeviceWaitIdle(bindDevice);
    vkFreeCommandBuffers(bindDevice, bindCommandPool, 1, &copyCommandBuffer);
}

void BufferManager::createVertexBuffer(size_t bufferSize, const void *verticesBufferData) {
    VkBuffer vertexBuffer{};
    VkDeviceMemory deviceBufferMemory{};
    createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBuffer,deviceBufferMemory);

    void *data;
    vkMapMemory(bindDevice, deviceBufferMemory,0, bufferSize,0, &data);
    memcpy(data, verticesBufferData, bufferSize);
    vkUnmapMemory(bindDevice, deviceBufferMemory);
    createdBuffers.emplace_back(vertexBuffer, deviceBufferMemory);
}

void BufferManager::createVertexBufferWithStagingBuffer(size_t bufferSize, const void *verticesBufferData) {
    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingBufferMemory{};
    createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,stagingBufferMemory);

    void *data;
    vkMapMemory(bindDevice, stagingBufferMemory,0, bufferSize,0, &data);
    memcpy(data, verticesBufferData, bufferSize);
    vkUnmapMemory(bindDevice, stagingBufferMemory);

    VkBuffer vertexBuffer{};
    VkDeviceMemory deviceBufferMemory{};
    createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, deviceBufferMemory);


    copyBuffer(stagingBuffer,vertexBuffer,bufferSize);
    // free the staging
    vkDestroyBuffer(bindDevice,stagingBuffer,nullptr);
    vkFreeMemory(bindDevice,stagingBufferMemory,nullptr);

    // manage our vertex buffer in array
    createdBuffers.emplace_back(vertexBuffer, deviceBufferMemory);
}

void BufferManager::createIndexBuffer(size_t indexBufferSize, const void *indicesData) {
    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingMemory{};
    createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void *data;
    vkMapMemory(bindDevice, stagingMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indicesData, indexBufferSize);
    vkUnmapMemory(bindDevice, stagingMemory);

    VkBuffer indexBuffer{};
    VkDeviceMemory indexMemory{};
    createBuffer(indexBufferSize,VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,indexBuffer, indexMemory);
    copyBuffer(stagingBuffer, indexBuffer, indexBufferSize);

    // free staging
    vkDestroyBuffer(bindDevice,stagingBuffer,nullptr);
    vkFreeMemory(bindDevice,stagingMemory,nullptr);

    // manage our index buffer in array
    createdIndexedBuffers.emplace_back(indexBuffer, indexMemory);
}


