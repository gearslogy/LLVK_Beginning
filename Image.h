//
// Created by liuyangping on 2024/5/28.
//

#ifndef IMAGES_H
#define IMAGES_H
#include <vulkan/vulkan.h>
#include <string>


struct ImageAndMemory {
    VkImage image{};
    VkDeviceMemory memory{};
};

// ImageFunction
struct FnImage{
    static VkImageView createImageView(VkDevice device,
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspectFlags) ;
    // need manully destory:vkDestoryImage() vkDestoryMemory()
    static ImageAndMemory createImageAndMemory(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t width, uint32_t height,
        VkFormat format,
        VkImageTiling tiling,               // VK_IMAGE_TILING_OPTIMAL
        VkImageUsageFlags usageFlags,       // VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        VkMemoryPropertyFlags propertyFlags // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    // need manully destory:vkDestoryImage() vkDestoryMemory()
    static ImageAndMemory createTexture( VkPhysicalDevice physicalDevice,
        VkDevice device,
        const VkCommandPool &poolToCopyStagingToImage,
        const VkQueue &queueToSubmitCopyStagingToImageCommand,
        const std::string &filePath);

    static void transitionImageLayout(VkDevice device,
        VkCommandPool pool,
        VkQueue queue,
        VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout);

    static void copyBufferToImage(VkDevice device,
        const VkCommandPool &pool, const VkQueue &queue,
        VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) ;

    // need manully destory:vkDestroySampler(device, textureSampler, nullptr);
    static VkSampler createImageSampler(VkPhysicalDevice physicalDevice,
        VkDevice device);

};




#endif //IMAGES_H
