//
// Created by liuyangping on 2024/5/28.
//
#include <stdexcept>
#include "LLVK_Image.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#include <vma/vk_mem_alloc.h>
#include "Utils.h"
#include "BufferManager.h"
#include "CommandManager.h"

LLVK_NAMESPACE_BEGIN
VkImageViewCreateInfo FnImage::imageViewCreateInfo(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;     // (COLOR_BIT) VK_IMAGE_ASPECT_COLOR_BIT VK_IMAGE_ASPECT_DEPTH_BIT ...
    viewCreateInfo.subresourceRange.baseMipLevel = 0; // only look the level 0
    viewCreateInfo.subresourceRange.levelCount = 1; // number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1; // number of array levels to view
    return viewCreateInfo;
}


void FnImage::createImageView(VkDevice device,
                              VkImage image,
                              VkFormat format,
                              VkImageAspectFlags aspectFlags, uint32_t mipLvels, uint32_t layerCount, VkImageView &view) {
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
    viewCreateInfo.subresourceRange.levelCount = mipLvels; // number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = layerCount; // number of array levels to view

    if (auto ret = vkCreateImageView(device, &viewCreateInfo, nullptr, &view); ret != VK_SUCCESS) {
        throw std::runtime_error{"ERROR create the image view"};
    }
}

void FnImage::createImageView(VkDevice device, const VkImageViewCreateInfo &createInfo, VkImageView &view) {
    if (auto ret = vkCreateImageView(device, &createInfo, nullptr, &view); ret != VK_SUCCESS) {
        throw std::runtime_error{"ERROR create the image view"};
    }
}


VkImageCreateInfo FnImage::imageCreateInfo(uint32_t width, uint32_t height) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage =  VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    return imageInfo;
}


VkMemoryAllocateInfo FnImage::imageAllocateInfo(VkPhysicalDevice physicalDevice,VkDevice device,  const VkImage &image,VkMemoryPropertyFlags propertyFlags) {
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, propertyFlags);
    return allocInfo;

}

void FnImage::createImageAndMemory(VkPhysicalDevice physicalDevice,
                                             VkDevice device,
                                             uint32_t width, uint32_t height,
                                             uint32_t mipLevels,
                                             uint32_t layerCount,
                                             VkFormat format,
                                             VkImageTiling tiling,
                                             VkImageUsageFlags usageFlags,
                                             VkMemoryPropertyFlags propertyFlags, VkImage &image, VkDeviceMemory &memory) {


    VkImageCreateInfo imageInfo= imageCreateInfo(width, height);
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = layerCount;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usageFlags;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }
    VkMemoryAllocateInfo allocInfo = imageAllocateInfo(physicalDevice, device, image, propertyFlags);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate image memory!");
    vkBindImageMemory(device, image, memory, 0);

}

void FnImage::createImageAndMemory(VkPhysicalDevice physicalDevice, VkDevice device,
    const VkImageCreateInfo &cio,VkMemoryPropertyFlags propertyFlags,
    VkImage &image, VkDeviceMemory &memory) {
    if (vkCreateImage(device, &cio, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }
    VkMemoryAllocateInfo allocInfo = imageAllocateInfo(physicalDevice, device,image, propertyFlags);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
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
    // caculate num mip levels
    auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

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
    /*注意这个要换
    ret = createImageAndMemory(physicalDevice, device, texWidth, texHeight, mipLevels,
                               format, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);*/
    // Change this to add VK_IMAGE_USAGE_TRANSFER_SRC_BIT. mipmap can be generated as a target or source.....
    createImageAndMemory(physicalDevice, device, texWidth, texHeight, mipLevels, 1,
                                   format, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ret.image, ret.memory);

    // 3. copy buffer to image
    transitionImageLayout(device, poolToCopyStagingToImage, queueToSubmitCopyStagingToImageCommand,
                          ret.image, format,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, 1);

        copyBufferToImage(device, poolToCopyStagingToImage, queueToSubmitCopyStagingToImageCommand, stagingBuffer,
                          ret.image, texHeight, texHeight);
    /*
    transitionImageLayout(device,poolToCopyStagingToImage,queueToSubmitCopyStagingToImageCommand,
        ret.image,format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
        */

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    generateMipmaps(physicalDevice, device,
        poolToCopyStagingToImage, queueToSubmitCopyStagingToImageCommand,
        ret.image,
        format, texWidth,texHeight, mipLevels);

    ret.mipLevels = mipLevels;
    return ret;
}


void FnImage::transitionImageLayout(VkDevice device,
                                    VkCommandPool pool, VkQueue queue,
                                    VkImage image, VkFormat format,
                                    VkImageLayout oldLayout, VkImageLayout newLayout,
                                    uint32_t mipLevels, uint32_t layerCount) {
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
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;


    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

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
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED and newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
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
    region.bufferOffset = 0; // this buffer offset. we only copy the stagging buffer. so it's zero.
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;// 只能复制特定的mip层
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1; // staging buffer  只有1层layercount?
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
    createInfo.maxLod = VK_LOD_CLAMP_NONE; // 1000.0f
    VkSampler textureSampler;
    if (vkCreateSampler(device, &createInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    return textureSampler;
}
VkSampler FnImage::createDepthSampler(VkDevice device) {
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = 1;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;//it only effects when the addressModeU/V/W=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
    // setting up mip
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 1.0;
    VkSampler textureSampler;
    if (vkCreateSampler(device, &createInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    return textureSampler;
}




VkFormat FnImage::findSupportedFormat(VkPhysicalDevice physicalDevice,
    const std::vector<VkFormat> &candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) {


    for(const auto &format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props );
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

bool FnImage::findDepthStencilFormat(VkPhysicalDevice physicalDevice) {
    return findSupportedFormat(
            physicalDevice,
           {VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_D16_UNORM_S8_UINT,},
           VK_IMAGE_TILING_OPTIMAL,
           VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
       );
}



VkFormat FnImage::findDepthFormat(VkPhysicalDevice physicalDevice) {
    return findSupportedFormat(
            physicalDevice,
    {VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D32_SFLOAT,VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_D16_UNORM_S8_UINT,VK_FORMAT_D16_UNORM},
           VK_IMAGE_TILING_OPTIMAL,
           VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
       );

}

void FnImage::generateMipmaps(
                            VkPhysicalDevice physicalDevice,
                            VkDevice device,
                            VkCommandPool pool,
                            VkQueue queue,
                            VkImage image,
                            VkFormat imageFormat,
                            int32_t texWidth, int32_t texHeight,
                            uint32_t mipLevels) {

    // image format supports linear blitting?
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }

    auto commandBuffer = FnCommand::beginSingleTimeCommand(device, pool);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;



    for (uint32_t i = 1; i < mipLevels; i++) {

        //std::cout << "process level:" << i << std::endl;
        barrier.subresourceRange.baseMipLevel = i - 1;// 永远用上一层
        // DST->SRC
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // 注意这里，以前必须是staging -> DST
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        // WRITE->READ(为了读取，然后复制出来到每一层level)
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        // 第一步，先把上一层 layout 从 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ---> VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

        // 第二步，类似复制， 从上一层到从0开始 -> (1,2,3,4....)
        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);

        // 第三步，当从上一层复制过来之后
        // 设置当层layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);
        // 每一次执行完，都给宽度高度/2
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;

    }

    // 由于最后一层没有执行转换VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

    FnCommand::endSingleTimeCommand(device, pool, queue, commandBuffer);
}








LLVK_NAMESPACE_END

