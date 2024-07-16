//
// Created by liuyangping on 2024/5/28.
//

#ifndef IMAGES_H
#define IMAGES_H
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

struct ImageAndMemory {
    VkImage image{};
    VkDeviceMemory memory{};
    uint32_t mipLevels{}; // using FnImage::createTexture() func will fill this field
};

// ImageFunction
struct FnImage {
    static VkImageView createImageView(VkDevice device,
                                       VkImage image,
                                       VkFormat format,
                                       VkImageAspectFlags aspectFlags,
                                       uint32_t mipLvels);

    // need manully destory:vkDestoryImage() vkDestoryMemory()
    static ImageAndMemory createImageAndMemory(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t width, uint32_t height,
        uint32_t mipLevels,
        VkFormat format,
        VkImageTiling tiling, // VK_IMAGE_TILING_OPTIMAL
        VkImageUsageFlags usageFlags, // VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        VkMemoryPropertyFlags propertyFlags// VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    // need manully destory:vkDestoryImage() vkDestoryMemory()
    static ImageAndMemory createTexture(VkPhysicalDevice physicalDevice,
                                        VkDevice device,
                                        const VkCommandPool &poolToCopyStagingToImage,
                                        const VkQueue &queueToSubmitCopyStagingToImageCommand,
                                        const std::string &filePath);

    static void transitionImageLayout(VkDevice device,
                                      VkCommandPool pool,
                                      VkQueue queue,
                                      VkImage image, VkFormat format,
                                      VkImageLayout oldLayout, VkImageLayout newLayout,
                                      uint32_t mipLevels);

    static void copyBufferToImage(VkDevice device,
                                  const VkCommandPool &pool, const VkQueue &queue,
                                  VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    // need manully destory:vkDestroySampler(device, textureSampler, nullptr);
    static VkSampler createImageSampler(VkPhysicalDevice physicalDevice,
                                        VkDevice device);

    static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates,
                                      VkImageTiling tiling,
                                      VkFormatFeatureFlags features);

    static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
    static bool findDepthStencilFormat(VkPhysicalDevice physicalDevice);

    static bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }



    static void generateMipmaps(
                                VkPhysicalDevice physicalDevice,
                                VkDevice device,
                                VkCommandPool pool,
                                VkQueue queue,
                                VkImage image,
                                VkFormat imageFormat,
                                int32_t texWidth, int32_t texHeight,
                                uint32_t mipLevels);

};




#endif //IMAGES_H
