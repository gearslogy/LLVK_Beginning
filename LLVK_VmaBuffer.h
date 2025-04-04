﻿//
// Created by liuya on 8/4/2024.
//

#pragma once

#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
#include <vma/vk_mem_alloc.h>
#include "CommandManager.h"
#include <ktxvulkan.h>
#include <cassert>
#include <libs/magic_enum.hpp>
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
        assert(allocator!=VK_NULL_HANDLE);
        assert(allocator!=VK_NULL_HANDLE);
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

struct VmaSSBOBuffer {
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
    // bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT  for indirect command buffer
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
        VkDeviceSize memSize = bufferSize;
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
            createIndexedBuffers.emplace_back(dstBuffer, dstAllocation , bufferSize);
        }
        else if constexpr (bufferUsage == (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT )){
            createVertexBuffers.emplace_back(dstBuffer, dstAllocation, bufferSize);
        }
        else if constexpr (bufferUsage == (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)) {
            createIndirectBuffers.emplace_back(dstBuffer, dstAllocation, bufferSize);
        }
        else {
            static_assert(not ALWAYS_TRUE, "only support index or vertex");
        }
    }




    void cleanup();

    std::vector<VmaBufferAndAllocation> createVertexBuffers;
    std::vector<VmaBufferAndAllocation> createIndexedBuffers;
    std::vector<VmaBufferAndAllocation> createIndirectBuffers;

};


// --- Image Op ---
struct FnVmaImage {
    static void createImageAndAllocation(const VmaBufferRequiredObjects &reqObj,
                                             uint32_t width, uint32_t height,
                                             uint32_t mipLevels, uint32_t layerCount,
                                             const VkFormat format,
                                             VkImageTiling tiling,
                                             VkImageUsageFlags usageFlags,
                                             bool canMapping,
                                             VkImage &image, VmaAllocation &imageAllocation);
    static void createImageAndAllocation(const VmaBufferRequiredObjects &reqObj,
        const VkImageCreateInfo &createInfo, bool canMapping, VkImage &image, VmaAllocation &allocation );

    // MIP generated!
    static void createTexture(const VmaBufferRequiredObjects &reqObj,const VkFormat format,
        const std::string &filePath,
        VkImage &image, VmaAllocation &allocation,uint32_t &createdMipLevels
        );
};

struct IVmaUBOTexture {
    VkImage image{};
    VmaAllocation imageAllocation{};
    VkImageView view{};
    VkFormat format{};
    VkDescriptorImageInfo descImageInfo{}; // for writeDescriptorSet
};


// for G-Buffer usage. also can will use for the Depth attachment
struct VmaAttachment : IVmaUBOTexture {
    void create(uint32_t width, uint32_t height,
        const VkFormat &attachFormat,
        const VkSampler & sampler,
        const VkImageUsageFlags &usage);
    //only create as depth attachment
    void createDepth32(uint32_t width, uint32_t height,const VkSampler & sampler);
    void create2dArrayDepth32(uint32_t width, uint32_t height, uint32_t layerCount, const VkSampler & sampler);
    void cleanup();
    VmaBufferRequiredObjects requiredObjects{};
    bool isValid{false};
};




// depth and stencil attachment
struct VmaDepthStencilAttachment {
    VkImage image{};
    VmaAllocation imageAllocation{};
    VkImageView depthView{};
    VkImageView stencilView{};
    VkFormat format{VK_FORMAT_D32_SFLOAT_S8_UINT}; // ALWAYS THIS FORMAT!
    VkDescriptorImageInfo descDepthImageInfo{};
    VkDescriptorImageInfo descStencilImageInfo{};

    void create(uint32_t width, uint32_t height,const VkSampler & sampler);
    void cleanup();
    VmaBufferRequiredObjects requiredObjects{};
};




// read tga/jpg/ ... etc use
struct VmaUBOTexture : IVmaUBOTexture {
    VmaBufferRequiredObjects requiredObjects{};
    void create(const std::filesystem::path &file, const VkSampler &sampler,const VkFormat &imageFormat = VK_FORMAT_R8G8B8A8_SRGB);
    void cleanup();
    bool isValid{false};
};

// custom resolve the ktx2 image. !todo
struct VmaUBOKTXTexture: IVmaUBOTexture {
    VmaBufferRequiredObjects requiredObjects{};
    void create(const std::string &file, VkSampler sampler);
    void cleanup();
};

struct VmaUBOKTX2Texture: IVmaUBOTexture {
    VmaBufferRequiredObjects requiredObjects{};
    void create(const std::string &file, VkSampler sampler);
    void cleanup();
private:
    ktxVulkanTexture ktx_vk_texture{};
};



LLVK_NAMESPACE_END



