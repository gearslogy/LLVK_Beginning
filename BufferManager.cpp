//
// Created by star on 5/7/2024.
//

#include "BufferManager.h"
#include <stdexcept>
#include "Utils.h"
#include "CommandManager.h"



LLVK_NAMESPACE_BEGIN
void FnBuffer::createBuffer(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memProperties,
    VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if(vkCreateBuffer(device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error{"can not create buffer"};
    }
    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(device, buffer, &memReqs);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memReqs.size;
    allocateInfo.memoryTypeIndex =  findMemoryType(physicalDevice, memReqs.memoryTypeBits, memProperties);
    vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory);
    vkBindBufferMemory(device, buffer, bufferMemory,0);// bind buffer <<--->> buffer memory
}

void FnBuffer::copyBuffer(VkDevice device,
                          VkQueue copyCommandQueue,
                          VkCommandPool copyCommandPool,
                          VkBuffer srcBuffer,
                          VkBuffer dstBuffer,
                          VkDeviceSize size) {
    VkCommandBuffer cmd = FnCommand::beginSingleTimeCommand(device, copyCommandPool);
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
    FnCommand::endSingleTimeCommand(device, copyCommandPool, copyCommandQueue, cmd);
}

void BufferAndMemory::cleanup(VkDevice device) {
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device,memory, nullptr);
}



void BufferManager::cleanup() {
    for(auto &bf : createdBuffers)
        bf.cleanup(bindDevice);
    createdBuffers.clear();
    for(auto &bf:createdIndexedBuffers) {
        bf.cleanup(bindDevice);
    }
    createdIndexedBuffers.clear();
}


void BufferManager::createVertexBuffer(size_t bufferSize, const void *verticesBufferData) {
    VkBuffer vertexBuffer{};
    VkDeviceMemory deviceBufferMemory{};
    FnBuffer::createBuffer(bindPhysicalDevice, bindDevice,bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
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
    FnBuffer::createBuffer(bindPhysicalDevice, bindDevice,bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,stagingBufferMemory);

    void *data;
    vkMapMemory(bindDevice, stagingBufferMemory,0, bufferSize,0, &data);
    memcpy(data, verticesBufferData, bufferSize);
    vkUnmapMemory(bindDevice, stagingBufferMemory);

    VkBuffer vertexBuffer{};
    VkDeviceMemory deviceBufferMemory{};
    FnBuffer::createBuffer(bindPhysicalDevice, bindDevice,bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, deviceBufferMemory);


    FnBuffer::copyBuffer(bindDevice,bindQueue,bindCommandPool,stagingBuffer,vertexBuffer,bufferSize);
    // free the staging
    vkDestroyBuffer(bindDevice,stagingBuffer,nullptr);
    vkFreeMemory(bindDevice,stagingBufferMemory,nullptr);

    // manage our vertex buffer in array
    createdBuffers.emplace_back(vertexBuffer, deviceBufferMemory);
}

void BufferManager::createIndexBuffer(size_t indexBufferSize, const void *indicesData) {
    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingMemory{};
    FnBuffer::createBuffer(bindPhysicalDevice, bindDevice,indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void *data;
    vkMapMemory(bindDevice, stagingMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indicesData, indexBufferSize);
    vkUnmapMemory(bindDevice, stagingMemory);

    VkBuffer indexBuffer{};
    VkDeviceMemory indexMemory{};
    FnBuffer::createBuffer(bindPhysicalDevice, bindDevice,indexBufferSize,VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,indexBuffer, indexMemory);
    FnBuffer::copyBuffer(bindDevice,bindQueue,bindCommandPool,stagingBuffer, indexBuffer, indexBufferSize);

    // free staging
    vkDestroyBuffer(bindDevice,stagingBuffer,nullptr);
    vkFreeMemory(bindDevice,stagingMemory,nullptr);

    // manage our index buffer in array
    createdIndexedBuffers.emplace_back(indexBuffer, indexMemory);
}

LLVK_NAMESPACE_END
