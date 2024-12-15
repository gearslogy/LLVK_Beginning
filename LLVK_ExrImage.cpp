//
// Created by liuya on 12/16/2024.
//
#define TINYEXR_IMPLEMENTATION
#include <LLVK_Utils.hpp>
#include "libs/tinyexr/tinyexr.h"
#include "libs/tinyexr/deps/miniz/miniz.h"
#include "LLVK_ExrImage.h"

LLVK_NAMESPACE_BEGIN

void VmaUBOExrRGBATexture::cleanup() {

}

void VmaUBOExrRGBATexture::create(const std::string &file, const VkSampler &sampler) {
    const auto format = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
}




LLVK_NAMESPACE_END