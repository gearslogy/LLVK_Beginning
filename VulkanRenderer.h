
#ifndef CP_01_VULKANRENDERER_H
#define CP_01_VULKANRENDERER_H

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "RenderPass.h"
#include "Utils.h"
#include "VulkanValidation.h"
#include "Swapchain.h"

class VulkanRenderer {
public :
    VulkanRenderer();
    int init(GLFWwindow * rh);
    void cleanup();
private:
    GLFWwindow  *window;
    VkInstance instance{};
    bool enableValidation{true};
    VkDebugReportCallbackEXT reportCallback;
    struct MainDevice{
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
    };
    MainDevice mainDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    VkSurfaceKHR surfaceKhr;



    // Objects
    DebugV2::CustomDebug simpleDebug;
    Pipeline simplePipeline;
    RenderPass simplePass;
    Swapchain simpleSwapchain;

    // create functions
    void createInstance();
    void createDebugCallback();
    void createLogicDevice();
    void createSurface();
    void createSwapChain();
    void createRenderpass();
    void createPipeline();
    void getPhysicalDevice();

    // support functions
    void checkDeviceExtensionSupport(const std::vector<const char*> &checkExtensions) const;
    static void checkInstanceExtensionSupport(const std::vector<const char *> &checkExtensions);
    [[nodiscard]] bool checkDeviceSuitable(const VkPhysicalDevice &device) const ;

private:
    inline static const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


};
#endif //CP_01_VULKANRENDERER_H
