//
// Created by star on 5/3/2024.
//

#ifndef DEVICE_H
#define DEVICE_H


#include <vulkan/vulkan.h>
#include <vector>
struct Device {
    // need to transfer from VulkanRenderer class
    VkInstance bindInstance;
    VkSurfaceKHR bindSurface;

    // created object
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;

    void init();
    void cleanup();

private:
    void getPhysicalDevice();
    void createLogicDevice();
    [[nodiscard]] bool checkDeviceSuitable(const VkPhysicalDevice &device) const;
    // support functions
    static bool checkDeviceExtensionSupport(VkPhysicalDevice device,
        const std::vector<const char*> &checkExtensions) ;
private:
    inline static const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};



#endif //DEVICE_H
