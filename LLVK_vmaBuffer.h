//
// Created by liuya on 8/4/2024.
//



#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
#include "vma/vk_mem_alloc.h"
#include "CommandManager.h"
LLVK_NAMESPACE_BEGIN
namespace FnVmaBuffer {

    template<bool need_mapping>
    inline VkResult createBuffer(
    VkDevice device,
    VmaAllocator allocator,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkBuffer& buffer,
    VmaAllocation& allocation)
    {
        // 创建缓冲区信息结构体
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // VMA 分配信息结构体
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if constexpr (need_mapping){
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        }
        // 创建缓冲区和分配内存
        VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
        return result;
    }
    // 销毁缓冲区
    inline void destroyBuffer(VkDevice device,
        VmaAllocator allocator,
        VkBuffer buffer,
        VmaAllocation allocation)
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    // 复制缓冲区数据
    inline void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer cmd = FnCommand::beginSingleTimeCommand(device, commandPool);
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
        FnCommand::endSingleTimeCommand(device, commandPool, graphicsQueue, cmd);
    }



};

struct VmaBufferManagerRequiredObjects {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue queue;
    VmaAllocator allocator;
};

struct VmaBuffer {
    VkBuffer buffer{};
    VmaAllocation allocation{};

    // when it's a ubo
    VkDeviceSize deviceSize{}; // when created
    void *mapped;
    VkDescriptorBufferInfo descBufferInfo{};


};
struct VmaBufferManager {
    VmaBufferManagerRequiredObjects requiredObjects;

    // bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT    for vertex
    // bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT     for index
    template<VkBufferUsageFlagBits bufferUsage>
    void createVertexBufferWithStagingBuffer(size_t bufferSize, const void *verticesBufferData) {
        VkBuffer stagingBuffer{};
        VmaAllocation stagingAllocation{};

        VkResult result = FnVmaBuffer::createBuffer<true>(requiredObjects.device,
            requiredObjects.allocator,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            stagingBuffer, stagingAllocation);
        if (result != VK_SUCCESS) throw std::runtime_error{"ERROR create stagging vma buffer"};


        // 复制数据到 staging buffer
        void* mappedData;
        vmaMapMemory(requiredObjects.allocator, stagingAllocation, &mappedData);
        memcpy(mappedData, verticesBufferData, bufferSize);
        vmaUnmapMemory(requiredObjects.allocator, stagingAllocation);

        // vertex buffer

        // 创建 vertex buffer
        VkBuffer vertexBuffer;
        VmaAllocation vertexAllocation;
        result = FnVmaBuffer::createBuffer<false>(requiredObjects.device,
            requiredObjects.allocator, bufferSize,
            bufferUsage,
            vertexBuffer, vertexAllocation);
        if (result != VK_SUCCESS) throw std::runtime_error{"ERROR create vma vertex buffer"};


        FnVmaBuffer::copyBuffer(requiredObjects.device,
            requiredObjects.commandPool,
            requiredObjects.queue,
            stagingBuffer, vertexBuffer,
            bufferSize);
        // 清理 staging buffer
        FnVmaBuffer::destroyBuffer(requiredObjects.device, requiredObjects.allocator, stagingBuffer, stagingAllocation);
    }



};
LLVK_NAMESPACE_END



