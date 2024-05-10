//
// Created by star on 5/7/2024.
//

#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H

#include <vulkan/vulkan.h>
#include <vector>

struct BufferAndMemory {
    VkBuffer buffer;
    VkDeviceMemory memory;
};
struct BufferManager {
    VkDevice bindDevice;
    VkPhysicalDevice bindPhysicalDevice;
    VkCommandPool bindCommandPool;// current use graphicsCommandPool
    VkQueue bindQueue;
    void createBuffer(VkDeviceSize size, // real size.
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags memProperties,
                      VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    std::vector<BufferAndMemory> createdBuffers; // use cleanup() to clean
    void cleanup();
    // The following functions are just for learning
    void createVertexBuffer(size_t bufferSize, const void *verticesBufferData);
    void createVertexBufferWithStagingBuffer(size_t bufferSize, const void *verticesBufferData);

    // create index vertex buffer
    void createIndexBuffer(size_t indexBufferSize, const void *indicesData);
    std::vector<BufferAndMemory> createdIndexedBuffers;
};



#endif //BUFFERMANAGER_H
