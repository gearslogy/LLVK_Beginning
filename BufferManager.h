//
// Created by star on 5/7/2024.
//

#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H

#include <vulkan/vulkan.h>
#include <vector>
#include "LLVK_SYS.hpp"
LLVK_NAMESPACE_BEGIN
struct FnBuffer {
    static void createBuffer(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkDeviceSize size, // real size.
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memProperties,
        VkBuffer &buffer,
        VkDeviceMemory &bufferMemory);

    static void copyBuffer(VkDevice device,
                           VkQueue copyCommandQueue,
                           VkCommandPool copyCommandPool,
                           VkBuffer srcBuffer,
                           VkBuffer dstBuffer,
                           VkDeviceSize size);
};

struct BufferAndMemory {
    VkBuffer buffer;
    VkDeviceMemory memory;

    void cleanup(VkDevice device);
    // when create
    VkDeviceSize size;
};

struct BufferManager {
    VkDevice bindDevice;
    VkPhysicalDevice bindPhysicalDevice;
    VkCommandPool bindCommandPool;// current use graphicsCommandPool
    VkQueue bindQueue;


    std::vector<BufferAndMemory> createdBuffers; // use cleanup() to clean
    void cleanup();
    // The following functions are just for learning
    void createVertexBuffer(size_t bufferSize, const void *verticesBufferData);
    void createVertexBufferWithStagingBuffer(size_t bufferSize, const void *verticesBufferData);
    

    // create index vertex buffer
    void createIndexBuffer(size_t indexBufferSize, const void *indicesData);
    std::vector<BufferAndMemory> createdIndexedBuffers;
};



template<typename vertex_t>
inline void createVertexAndIndexBuffer(auto &&bufferManager, auto &geometry) {
    static_assert(requires { geometry.vertices; });
    static_assert(requires { geometry.indices; });
    static_assert(requires { bufferManager.createdBuffers; });
    static_assert(requires { bufferManager.createdIndexedBuffers; });

    bufferManager.createVertexBufferWithStagingBuffer(sizeof(vertex_t) * geometry.vertices.size(),
                                                      geometry.vertices.data());
    bufferManager.createIndexBuffer(sizeof(uint32_t) * geometry.indices.size(),
                                    geometry.indices.data());

    // assgin right buffer-ptr
    auto &&verticesBuffers = bufferManager.createdBuffers; // return struct BufferAndMemory
    auto &&indicesBuffers = bufferManager.createdIndexedBuffers;
    geometry.verticesBuffer = verticesBuffers[verticesBuffers.size() - 1].buffer;
    geometry.indicesBuffer = indicesBuffers[indicesBuffers.size() - 1].buffer;
}



LLVK_NAMESPACE_END

#endif //BUFFERMANAGER_H
