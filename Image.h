//
// Created by liuyangping on 2024/5/28.
//

#ifndef IMAGES_H
#define IMAGES_H
#include <vulkan/vulkan.h>
struct FnImage{
    static VkImageView createImageView(VkDevice device,
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspectFlags) ;

};


class Image {

};



#endif //IMAGES_H
