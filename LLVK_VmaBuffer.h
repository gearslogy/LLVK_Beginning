//
// Created by liuya on 8/4/2024.
//



#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
#include "vma/vk_mem_alloc.h"
#include "CommandManager.h"

LLVK_NAMESPACE_BEGIN

namespace FnVmaBuffer {
    inline VkResult createBuffer(
    VkDevice device,
    VmaAllocator allocator,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    bool canMapping,
    VkBuffer& buffer,
    VmaAllocation& allocation)
    {
        // buffer struct same as vulkan api
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // vma struct CIO
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if (canMapping){
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        // create buffer and allocate memory
        VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
        return result;
    }
    // 销毁缓冲区
    inline void destroyBuffer(
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

struct VmaBufferRequiredObjects {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue queue;
    VmaAllocator allocator;
};

struct VmaBufferAndAllocation {
    VkBuffer buffer{};
    VmaAllocation allocation{};
    VkDeviceSize memorySize{};
};

struct VmaUBOBuffer {
    VmaBufferAndAllocation bufferAndAllocation{};
    VkDescriptorBufferInfo descBufferInfo{};
    VmaBufferRequiredObjects requiredObjects{};
    void *mapped{};
    void createAndMapping(VkDeviceSize bufferSize);
    void cleanup();
};


struct VmaSimpleGeometryBufferManager {
    VmaBufferRequiredObjects requiredObjects;

    // bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT    for vertex
    // bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT     for index
    template<VkBufferUsageFlags bufferUsage>
    void createBufferWithStagingBuffer(size_t bufferSize, const void *verticesBufferData) {
        VkBuffer stagingBuffer{};
        VmaAllocation stagingAllocation{};

        VkResult result = FnVmaBuffer::createBuffer(requiredObjects.device,
            requiredObjects.allocator,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,true,
            stagingBuffer, stagingAllocation);
        if (result != VK_SUCCESS) throw std::runtime_error{"ERROR create stagging vma buffer"};


        // 复制数据到 staging buffer
        void* mappedData;
        vmaMapMemory(requiredObjects.allocator, stagingAllocation, &mappedData);
        memcpy(mappedData, verticesBufferData, bufferSize);
        vmaUnmapMemory(requiredObjects.allocator, stagingAllocation);

        // vertex buffer

        // create vertex buffer  or  index buffer
        VkBuffer dstBuffer;
        VmaAllocation dstAllocation;
        result = FnVmaBuffer::createBuffer(requiredObjects.device,
            requiredObjects.allocator, bufferSize,
            bufferUsage,true,
            dstBuffer, dstAllocation);
        if (result != VK_SUCCESS) throw std::runtime_error{"ERROR create vma vertex buffer"};


        FnVmaBuffer::copyBuffer(requiredObjects.device,
            requiredObjects.commandPool,
            requiredObjects.queue,
            stagingBuffer, dstBuffer,
            bufferSize);
        // clean staging buffer
        FnVmaBuffer::destroyBuffer(requiredObjects.allocator, stagingBuffer, stagingAllocation);

        if constexpr (bufferUsage == (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT) ) {
            std::cout << "---------------------------create index buffer\n";
            createIndexedBuffers.emplace_back(dstBuffer, dstAllocation);
        }
        else if constexpr (bufferUsage == (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT )){
            std::cout << "---------------------------create vertex buffer\n";
            createVertexBuffers.emplace_back(dstBuffer, dstAllocation);
        }
        else {
            static_assert(not ALWAYS_TRUE, "only support index or vertex");
        }
    }

    void cleanup();

    std::vector<VmaBufferAndAllocation> createVertexBuffers;
    std::vector<VmaBufferAndAllocation> createIndexedBuffers;

};



struct FnVmaImage {
    static void createImageAndAllocation(const VmaBufferRequiredObjects &reqObj,
                                             uint32_t width, uint32_t height,
                                             uint32_t mipLevels,
                                             VkFormat format,
                                             VkImageTiling tiling,
                                             VkImageUsageFlags usageFlags,
                                             bool canMapping,
                                             VkImage &image, VmaAllocation &imageAllocation);
    static void createTexture(const VmaBufferRequiredObjects &reqObj,
        const std::string &filePath,
        VkImage &image, VmaAllocation &allocation,uint32_t &createdMipLevels
        );
};

struct VmaUBOTexture {
    VkImage image{};
    VmaAllocation imageAllocation{};
    VkImageView view{};
    VkDescriptorImageInfo descImageInfo{}; // for writeDescriptorSet

    VmaBufferRequiredObjects requiredObjects{};
    void create(const std::string &file, VkSampler sampler);
    void cleanup();
};


LLVK_NAMESPACE_END



