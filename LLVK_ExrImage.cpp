//
// Created by liuya on 12/16/2024.
//
#define TINYEXR_IMPLEMENTATION
#include <LLVK_Utils.hpp>
#include "libs/tinyexr/tinyexr.h"
#include "libs/tinyexr/deps/miniz/miniz.h"
#include "LLVK_ExrImage.h"

#include "LLVK_Image.h"

LLVK_NAMESPACE_BEGIN
void VmaUBOExrRGBATexture::cleanup() {
    vmaDestroyImage(requiredObjects.allocator, image, imageAllocation);
    vkDestroyImageView(requiredObjects.device, view, nullptr);
}

void VmaUBOExrRGBATexture::create(const std::string &file, const VkSampler &sampler) {
    const auto &[device,physicalDevice,commandPool, queue, allocator] = requiredObjects;
    constexpr auto format = VK_FORMAT_R32G32B32A32_SFLOAT; // VK_FORMAT_R16G16B16A16_SFLOAT
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    parseExrRGBAHeader(file);
    int width,height{};
    auto *pixels = parseExrRGBAData(file, width, height);
    VkDeviceSize imageSize = width * height * 4;

    // 1. create staging
    VkBuffer stagingBuffer{};
    VmaAllocation stagingBufferAllocation{};
    FnVmaBuffer::createBuffer(device, allocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        true,stagingBuffer, stagingBufferAllocation);
    // mapping staging
    void* data;
    vmaMapMemory(allocator, stagingBufferAllocation, &data);
    memcpy(data, pixels, imageSize);
    vmaUnmapMemory(allocator, stagingBufferAllocation);
    constexpr auto mipLevels = 1;
    // 2. create target
    FnVmaImage::createImageAndAllocation(requiredObjects, width, height,
        mipLevels,1,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        false,
        image, imageAllocation );

    // 3. copy buffer to image
    FnImage::transitionImageLayout(device, commandPool, queue,
                          image, format,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels,1);
    FnImage::copyBufferToImage(device, commandPool, queue, stagingBuffer,
                          image, width, height);

    // 4. create image view
    FnImage::createImageView(requiredObjects.device, image, format,aspect,1, 1, view);
    descImageInfo.sampler = sampler;
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // finally need transition to this layout
    descImageInfo.imageView = view;
    vmaDestroyBuffer(allocator, stagingBuffer,stagingBufferAllocation);
    free(data);
}


void VmaUBOExrRGBATexture::parseExrRGBAHeader(const std::string &file) {
    EXRVersion exr_version;
    const char *err = nullptr;
    int ret = ParseEXRVersionFromFile(&exr_version, file.data());
    if (ret != 0) {
        fprintf(stderr, "Invalid EXR file or version\n");
        return;
    }

    EXRHeader header;
    InitEXRHeader(&header);
    ret = ParseEXRHeaderFromFile(&header, &exr_version, file.data(), &err);
    if (ret != 0) {
        fprintf(stderr, "Parse single part exr header failed: %s\n, quit loader", err);
        FreeEXRErrorMessage(err);
        return;
    }
    if (header.num_channels != 4) {
        FreeEXRHeader(&header);
        throw std::runtime_error("wrong number of channels expect:4 RGBA");
    }
    FreeEXRHeader(&header);
}

float *VmaUBOExrRGBATexture::parseExrRGBAData(const std::string &file, int &width, int &height) {
    float* out; // width * height * RGBA
    const char *err = nullptr;
    auto ret = LoadEXR(&out, &width, &height, file.data(), &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            fprintf(stderr, "ERR : %s\n", err);
            FreeEXRErrorMessage(err); // release memory of error message.
        }
        throw std::runtime_error("failed to parse EXR file");
    }
    return out;
}





LLVK_NAMESPACE_END