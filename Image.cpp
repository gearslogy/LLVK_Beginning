//
// Created by liuyangping on 2024/5/28.
//
#include <stdexcept>
#include "Image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#include "libs/VulkanMemoryAllocator-3.0.1/include/vk_mem_alloc.h"
#include "Utils.h"
#include "BufferManager.h"
#include "CommandManager.h"
VkImageView FnImage::createImageView(VkDevice device,
                                     VkImage image,
                                     VkFormat format,
                                     VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    // (COLOR_BIT) VK_IMAGE_ASPECT_COLOR_BIT VK_IMAGE_ASPECT_DEPTH_BIT ...
    viewCreateInfo.subresourceRange.baseMipLevel = 0; // only look the level 0
    viewCreateInfo.subresourceRange.levelCount = 1; // number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1; // number of array levels to view

    VkImageView view{};
    if (auto ret = vkCreateImageView(device, &viewCreateInfo, nullptr, &view); ret != VK_SUCCESS) {
        throw std::runtime_error{"ERROR create the image view"};
    }
    return view;
}


ImageAndMemory FnImage::createImageAndMemory( VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t width,uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usageFlags,
    VkMemoryPropertyFlags propertyFlags) {

    ImageAndMemory imageAndMemory{};

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usageFlags;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageInfo, nullptr, &imageAndMemory.image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, imageAndMemory.image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice,memRequirements.memoryTypeBits, propertyFlags);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageAndMemory.memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    vkBindImageMemory(device, imageAndMemory.image, imageAndMemory.memory, 0);
    return imageAndMemory;
}

ImageAndMemory FnImage::createTexture(VkPhysicalDevice physicalDevice, VkDevice device,
                                      const VkCommandPool &poolToCopyStagingToImage,
                                      const VkQueue &queueToSubmitCopyStagingToImageCommand,
                                      const std::string &filePath) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    // 1.Create staging
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    FnBuffer::createBuffer(physicalDevice, device,
                           imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           stagingBuffer, stagingBufferMemory);
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);

    // 2.create image and memory
    ImageAndMemory ret{};
    constexpr VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    ret = createImageAndMemory(physicalDevice, device, texWidth, texHeight,
                               format, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


    // 3. copy buffer to image
    transitionImageLayout(device, poolToCopyStagingToImage, queueToSubmitCopyStagingToImageCommand,
                          ret.image, format,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(device, poolToCopyStagingToImage, queueToSubmitCopyStagingToImageCommand, stagingBuffer,
                          ret.image, texHeight, texHeight);
    transitionImageLayout(device,poolToCopyStagingToImage,queueToSubmitCopyStagingToImageCommand,
        ret.image,format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    return ret;
}


void FnImage::transitionImageLayout(VkDevice device,
                                    VkCommandPool pool, VkQueue queue,
                                    VkImage image, VkFormat format,
                                    VkImageLayout oldLayout, VkImageLayout newLayout) {
    // copy staging -> image
    auto commandBuffer = FnCommand::beginSingleTimeCommand(device, pool);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;


    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    /*
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED and newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL and newLayout ==
               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else throw std::runtime_error{"unsupported layout transition"};*/
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
          commandBuffer,
          sourceStage, destinationStage,
          0,
          0, nullptr,
          0, nullptr,
          1, &barrier
      );
    FnCommand::endSingleTimeCommand(device, pool, queue, commandBuffer);
}
// staging->image
void FnImage::copyBufferToImage(VkDevice device, const VkCommandPool &pool,
                                const VkQueue &queue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    auto commandBuffer = FnCommand::beginSingleTimeCommand(device, pool);
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    FnCommand::endSingleTimeCommand(device, pool, queue, commandBuffer);
}

VkSampler FnImage::createImageSampler(VkPhysicalDevice physicalDevice,VkDevice device) {
    VkPhysicalDeviceProperties gpuProps{};
    vkGetPhysicalDeviceProperties(physicalDevice, &gpuProps);
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = gpuProps.limits.maxSamplerAnisotropy;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;//it only effects when the addressModeU/V/W=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
    // setting up mip
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;
    VkSampler textureSampler;
    if (vkCreateSampler(device, &createInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    return textureSampler;
}



