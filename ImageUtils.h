//
// Created by star on 4/29/2024.
//

#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <vulkan/vulkan.h>

struct ImageUtils {
    static VkImageView createImageView(VkDevice device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags) ;
};



#endif //IMAGEUTILS_H
