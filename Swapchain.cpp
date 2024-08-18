//
// Created by star on 4/29/2024.
//

#include "Swapchain.h"
#include "Utils.h"
#include "libs/magic_enum.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include "LLVK_Image.h"
LLVK_NAMESPACE_BEGIN
void Swapchain::cleanup() {
    // swapchain里的 imageView得手动删除，image会被swapchain自动删除
    for(auto spcImg : swapChainImages) {
        vkDestroyImageView(bindLogicDevice, spcImg.imageView, nullptr);
    }
    swapChainImages.clear();
    vkDestroySwapchainKHR(bindLogicDevice, swapchain, nullptr);
}

void Swapchain::init() {
    #undef min
    auto [capabilities, formatList, presentModeList] = getSwapChainDetails(bindSurface,bindPhyiscalDevice);
    auto [format, colorSpace] =  chooseBestSurfaceFormat(formatList);  // VkSurfaceFormatKHR
    const auto surfacePresentationMode = chooseBestPresentationMode(presentModeList);
    auto minImageCount = capabilities.minImageCount  + 1;
    const auto maxImageCount = capabilities.maxImageCount;
    minImageCount = std::min(minImageCount, maxImageCount);
    const VkExtent2D extent = chooseSwapExtent(capabilities);
    std::cout << "swapChain surface format:" << magic_enum::enum_name(format) << " colorSpace:" << magic_enum::enum_name(colorSpace) << std::endl;
    std::cout << "swapChain present mode:" << magic_enum::enum_name(surfacePresentationMode) <<std::endl;
    std::cout << "swapChain capabilities currentTransform:" << magic_enum::enum_name(capabilities.currentTransform) <<std::endl;

    VkSwapchainCreateInfoKHR createInfoKhr{};
    createInfoKhr.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfoKhr.imageFormat = format;
    createInfoKhr.imageColorSpace = colorSpace;
    createInfoKhr.surface = bindSurface;
    createInfoKhr.presentMode = surfacePresentationMode;
    createInfoKhr.minImageCount = minImageCount;
    createInfoKhr.imageExtent = extent;
    createInfoKhr.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfoKhr.imageArrayLayers = 1; // AI. 它定义了交换链图像的层级数量，这对于立体图像（例如3D）或者多重采样图像是非常有用的。
    createInfoKhr.preTransform = capabilities.currentTransform; //AI.  VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR 表示不对表面进行任何变换。这意味着图形将以其原始方向显示，不会进行旋转或镜像翻转。
    createInfoKhr.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfoKhr.clipped = VK_TRUE; // AI. 如果clipped设置为VK_TRUE，则表示在交换链图像的渲染中要裁剪掉在屏幕边界之外的像素，以提高性能。

    auto indices = getQueueFamilies(bindSurface,bindPhyiscalDevice);
    if(indices.graphicsFamily != indices.presentationFamily){
        uint32_t queueFamilyIndices[]  {
                static_cast<uint32_t >(indices.graphicsFamily),
                static_cast<uint32_t >(indices.presentationFamily)
        };
        createInfoKhr.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfoKhr.queueFamilyIndexCount = 2;
        createInfoKhr.pQueueFamilyIndices = queueFamilyIndices;
    }
    else{
        createInfoKhr.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfoKhr.queueFamilyIndexCount = 0;
        createInfoKhr.pQueueFamilyIndices = nullptr;
    }
    createInfoKhr.oldSwapchain = VK_NULL_HANDLE;
    auto result= vkCreateSwapchainKHR(bindLogicDevice, &createInfoKhr, nullptr, &swapchain);
    if(result != VK_SUCCESS){
        throw std::runtime_error("Failed create Swap chain");
    }
    swapChainFormat = format;
    swapChainExtent = extent;

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(bindLogicDevice, swapchain, &swapChainImageCount, nullptr);
    assert(swapChainImageCount == 3);
    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(bindLogicDevice, swapchain, &swapChainImageCount, images.data());

    for(const auto &img : images) {
        SwapChainImage scImg{};
        scImg.image = img;
        FnImage::createImageView(bindLogicDevice,img, swapChainFormat, VK_IMAGE_ASPECT_COLOR_BIT,1,1, scImg.imageView );
        swapChainImages.emplace_back(scImg);
    }
}

SwapChainDetails Swapchain::getSwapChainDetails(VkSurfaceKHR surface, VkPhysicalDevice device)  {
    auto ret = SwapChainDetails{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &ret.capabilities);
    uint32_t surfaceFormatCount{0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, nullptr);
    ret.formatList.resize(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, ret.formatList.data());

    uint32_t presentModeCount{0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    ret.presentModeList.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, ret.presentModeList.data());
    return ret;
}

VkSurfaceFormatKHR Swapchain::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &inputs) {
    for(const auto &[sFormat,sColorSpace] : inputs){
        bool cond1 = (sFormat == surfaceFormatChoose.format) or (sFormat == VK_FORMAT_B8G8R8A8_SNORM);
        bool cond2 = sColorSpace == surfaceFormatChoose.colorSpace;
        if(cond1 and cond2)
            return {sFormat, sColorSpace};
    }
    return surfaceFormatChoose;
}

VkPresentModeKHR Swapchain::chooseBestPresentationMode(const std::vector<VkPresentModeKHR> &inputs){
    for(const auto &pm : inputs)
        if(pm == presentModeChoose_primary) return pm;
    return presentModeChoose_secondary;
}


VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &input) const{
#undef max
    if(input.currentExtent.width != std::numeric_limits<uint32_t>::max() ){
        std::cout << "Swapchain::chooseSwapExtent:" << input.currentExtent.width <<" " << input.currentExtent.height << std::endl;
        return input.currentExtent; // 一般情况下 根据input 得到的就是窗口大小
    }
    else {
        int width, height = 0;
        glfwGetFramebufferSize(bindWindow, &width, &height );
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }
}

LLVK_NAMESPACE_END