//
// Created by liuya on 8/4/2024.
//

#include "LLVK_vmaBuffer.h"
#include "libs/stb_image.h"
#include "LLVK_Image.h"
LLVK_NAMESPACE_BEGIN
void VmaUBOBuffer::createAndMapping(VkDeviceSize bufferSize) {
    auto result = FnVmaBuffer::createBuffer(requiredObjects.device,
        requiredObjects.allocator,
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        true,
        bufferAndAllocation.buffer, bufferAndAllocation.allocation);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR create stagging vma buffer"};
    bufferAndAllocation.memorySize = bufferSize;
    descBufferInfo.buffer = bufferAndAllocation.buffer;
    descBufferInfo.offset = 0;
    descBufferInfo.range = bufferSize;
    vmaMapMemory(requiredObjects.allocator, bufferAndAllocation.allocation, &mapped);
}
void VmaUBOBuffer::cleanup() {
    vmaUnmapMemory(requiredObjects.allocator, bufferAndAllocation.allocation);
    FnVmaBuffer::destroyBuffer(requiredObjects.allocator ,bufferAndAllocation.buffer,bufferAndAllocation.allocation );
}


void VmaSimpleGeometryBufferManager::cleanup() {
    for(auto &vma : createVertexBuffers) {
        FnVmaBuffer::destroyBuffer(requiredObjects.allocator,vma.buffer, vma.allocation);
    }
    for(auto &vma : createIndexedBuffers) {
        FnVmaBuffer::destroyBuffer(requiredObjects.allocator,vma.buffer, vma.allocation);
    }
}

//  image
void FnVmaImage::createImageAndAllocation(const VmaBufferRequiredObjects &reqObj,
                                             uint32_t width, uint32_t height,
                                             uint32_t mipLevels,
                                             VkFormat format,
                                             VkImageTiling tiling,
                                             VkImageUsageFlags usageFlags,
                                             bool canMapping,
                                             VkImage &image, VmaAllocation &imageAllocation) {
    const auto & [device,physicalDevice,commandPool, queue, allocator] = reqObj;
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usageFlags;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if(canMapping) allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    if(vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &image, &imageAllocation, nullptr) !=VK_SUCCESS) {
        throw std::runtime_error{"can not create image and allocation"};
    }
}

void FnVmaImage::createTexture(const VmaBufferRequiredObjects &reqObj,
    const std::string &filePath,VkImage &image, VmaAllocation &allocation,
    uint32_t &createdMipLevels) {
    const auto &[device,physicalDevice,commandPool, queue, allocator] = reqObj;
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    // caculate num mip levels
    auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    createdMipLevels = mipLevels;
    // create staging
    VkBuffer stagingBuffer{};
    VmaAllocation stagingBufferAllocation{};
    FnVmaBuffer::createBuffer(device, allocator,imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        true,stagingBuffer, stagingBufferAllocation);
    // mapping stagging
    void* data;
    vmaMapMemory(allocator, stagingBufferAllocation, &data);
    memcpy(data, pixels, imageSize);
    vmaUnmapMemory(allocator, stagingBufferAllocation);
    stbi_image_free(pixels);

    // create our image and allocation

    constexpr VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    createImageAndAllocation(reqObj, texWidth, texHeight,
        mipLevels,format,VK_IMAGE_TILING_OPTIMAL,+
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        false,
        image, allocation );

    // 3. copy buffer to image
    FnImage::transitionImageLayout(device, commandPool, queue,
                          image, format,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

    FnImage::copyBufferToImage(device, commandPool, queue, stagingBuffer,
                      image, texHeight, texHeight);

    FnImage::generateMipmaps(physicalDevice, device,
        commandPool, queue,
        image,
        format,
        texWidth,texHeight,
        mipLevels);

    vmaDestroyBuffer(allocator, stagingBuffer,stagingBufferAllocation);
}

void VmaUBOTexture::create(const std::string &file, VkSampler sampler) {
    uint32_t createMipLevels{};
    FnVmaImage::createTexture(requiredObjects, file, image, imageAllocation,createMipLevels);
    FnImage::createImageView( requiredObjects.device, image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_ASPECT_COLOR_BIT,
            createMipLevels,view);
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descImageInfo.imageView = view;
    descImageInfo.sampler = sampler;
}

void VmaUBOTexture::cleanup() {
    vmaDestroyImage(requiredObjects.allocator, image, imageAllocation);
    vkDestroyImageView(requiredObjects.device, view, nullptr);
}




LLVK_NAMESPACE_END


