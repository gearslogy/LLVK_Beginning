
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
    VkSwapchainKHR swapChainKhr;
    std::vector<SwapChainImage> swapChainImages;

    // utility
    VkFormat swapChainFormat;
    VkExtent2D swapChainExtent;

    // Objects
    DebugV2::CustomDebug simpleDebug;
    Pipeline simplePipeline;
    RenderPass simplePass;


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

    // Getter
    [[nodiscard]] QueueFamilyIndices getQueueFamilies(const VkPhysicalDevice &device) const;
    [[nodiscard]] SwapChainDetails getSwapChainDetails(const VkPhysicalDevice &device) const;

private:
    // 其实直接使用这些足以。有必要检查下
    static inline constexpr VkSurfaceFormatKHR surfaceFormatChoose = {.format=VK_FORMAT_R8G8B8A8_UNORM,// unsigned normalized value [0-1]
            .colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    static inline constexpr VkPresentModeKHR presentModeChoose_primary{VK_PRESENT_MODE_MAILBOX_KHR};
    static inline constexpr VkPresentModeKHR presentModeChoose_secondary{VK_PRESENT_MODE_FIFO_KHR};
public:
    static VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &inputs) ;
    static VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR> &inputs);
    [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &input) const;
private:
    inline static const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
};
#endif //CP_01_VULKANRENDERER_H
