//
// Created by star on 4/29/2024.
//

#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include "Utils.h"
struct GLFWwindow;
struct Swapchain {
    // all of this is ref object
    VkPhysicalDevice bindPhyiscalDevice{};
    VkDevice bindLogicDevice{};
    VkSurfaceKHR bindSurface{};
    GLFWwindow *bindWindow{};

    // created object
    // utility
    VkFormat swapChainFormat;
    VkExtent2D swapChainExtent;
    VkSwapchainKHR swapchain;
    std::vector<SwapChainImage> swapChainImages;

    // simple functions
    void init();
    void cleanup();

    // utilities
    static SwapChainDetails getSwapChainDetails(VkSurfaceKHR surface, VkPhysicalDevice device)  ;
    static VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &inputs) ;
    static VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR> &inputs);
    [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &input) const;
private:
    // 其实直接使用这些足以。有必要检查下
    static inline constexpr VkSurfaceFormatKHR surfaceFormatChoose = {.format=VK_FORMAT_R8G8B8A8_UNORM,// unsigned normalized value [0-1]
            .colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    static inline constexpr VkPresentModeKHR presentModeChoose_primary{VK_PRESENT_MODE_MAILBOX_KHR};
    static inline constexpr VkPresentModeKHR presentModeChoose_secondary{VK_PRESENT_MODE_FIFO_KHR};
};



#endif //SWAPCHAIN_H
