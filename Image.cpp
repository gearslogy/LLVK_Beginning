//
// Created by liuyangping on 2024/5/28.
//
#include <stdexcept>
#include "Image.h"
VkImageView FnImage::createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags; // (COLOR_BIT) VK_IMAGE_ASPECT_COLOR_BIT VK_IMAGE_ASPECT_DEPTH_BIT ...
    viewCreateInfo.subresourceRange.baseMipLevel = 0;// only look the level 0
    viewCreateInfo.subresourceRange.levelCount = 1; // number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;    // number of array levels to view

    VkImageView view{};
    if (auto ret = vkCreateImageView(device, &viewCreateInfo, nullptr, &view ); ret!=VK_SUCCESS) {
        throw std::runtime_error{"ERROR create the image view"};
    }
    return view;
}
