//
// Created by star on 5/3/2024.
//

#ifndef DEVICE_H
#define DEVICE_H


#include <vulkan/vulkan.h>
#include <vector>
#include "LLVK_SYS.hpp"
#include "LLVK_Utils.hpp"
LLVK_NAMESPACE_BEGIN

struct Device {
    // need to transfer from VulkanRenderer class
    VkInstance bindInstance;
    VkSurfaceKHR bindSurface;

    // created object
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    // 1 2 3 queue is same
    VkQueue graphicsQueue;   // 1
    VkQueue presentationQueue; // 2
    VkQueue computeQueue;    // 3
    // only transfer
    VkQueue transferQueue;

    // cached QueueFamlilies
    QueueFamilyIndices queueFamilyIndices;
    void init();
    void cleanup();

private:
    void getPhysicalDevice();
    void createLogicDevice();
    [[nodiscard]] bool checkDeviceSuitable(const VkPhysicalDevice &device);
    // support functions
    static bool checkDeviceExtensionSupport(VkPhysicalDevice device,
        const std::vector<const char*> &checkExtensions) ;
    static uint32_t getMaxPushConstantsSize(VkPhysicalDevice device);
private:
    inline static  std::vector<const char*> deviceExtensions = {
        //// Extensions required by mesh shading
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME// Required by VK_KHR_spirv_1_4
    };
};

LLVK_NAMESPACE_END

#endif //DEVICE_H
