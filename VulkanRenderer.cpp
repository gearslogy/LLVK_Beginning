
//
// Created by liuyangping on 2024/3/27.
//

#include "VulkanRenderer.h"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <cassert>
#include <set>
#include "VulkanValidation.h"
#include "magic_enum.hpp"
#include <format>
#include <filesystem>
VulkanRenderer::VulkanRenderer() {}
int VulkanRenderer::init(GLFWwindow *rh) {
    window = rh;
    try{
        createInstance();
        createDebugCallback();
        createSurface();
        getPhysicalDevice();
        createLogicDevice();
        createSwapChain();
        createRenderpass();
        createPipeline();
    }
    catch (const std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
void VulkanRenderer::cleanup() {
    simplePass.cleanup();
    simplePipeline.cleanup();
    // swapchain里的 imageView得手动删除，image会被swapchain自动删除
    for(auto &spcImg : swapChainImages) {
        vkDestroyImageView(mainDevice.logicalDevice, spcImg.imageView, nullptr);
    }
    vkDestroySwapchainKHR(mainDevice.logicalDevice, swapChainKhr, nullptr);
    vkDestroySurfaceKHR(instance,surfaceKhr, nullptr);
    vkDestroyDevice(mainDevice.logicalDevice, nullptr);
    if(enableValidation){
        DebugV1::DestroyDebugReportCallbackEXT(instance, reportCallback, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
}
void VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "TestEngineApp",
            .applicationVersion = VK_VERSION_1_3,
            .pEngineName = "TestEngine",
            .engineVersion = VK_VERSION_1_3,
            .apiVersion = VK_API_VERSION_1_3,
    };

    uint32_t glfwExtPropsCount;
    const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtPropsCount);
    std::cout << "glfwGetRequiredInstanceExtensions :" << glfwExtPropsCount << std::endl;
    std::vector<const char *> extensionNamesList;
    for(int i=0;i<glfwExtPropsCount;i++){
        std::cout << "glfw extension:" <<glfwExtensionNames[i] << std::endl;
        extensionNamesList.emplace_back(glfwExtensionNames[i]);
    }
    if(enableValidation) {
        extensionNamesList.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME); // VK_EXT_debug_report, this is old method(v1)
        extensionNamesList.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);  // we will use here
    }


    checkInstanceExtensionSupport(extensionNamesList);
    //VkInstanceCreateInfo instanceCreateInfo; // CAUSE CRASH
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensionNamesList.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensionNamesList.data();

    std::vector<const char *> layerNames = {
            //"VK_LAYER_LUNARG_api_dump", // SUPPORT
            //"VK_LAYER_LUNARG_standard_validation" // NOT SUPPORTED
            "VK_LAYER_KHRONOS_validation"
    };
    if(enableValidation){
        instanceCreateInfo.ppEnabledLayerNames = layerNames.data();
        instanceCreateInfo.enabledLayerCount = layerNames.size();
    }

    // create instance
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if(result != VK_SUCCESS){
        throw std::runtime_error("Failed create vulkan instance");
    }
}

void VulkanRenderer::createDebugCallback() {
    if(not enableValidation) return;
    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
    callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.pfnCallback = DebugV1::debugFunction;
    callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                               VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;

    // Create debug callback with custom create function
    VkResult const result = DebugV1::CreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &reportCallback);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Debug Callback!");
}

void VulkanRenderer::checkInstanceExtensionSupport(const std::vector<const char *> &checkExtensions) {
    uint32_t allSupportInstanceExtensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &allSupportInstanceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> instancePropertiesList(allSupportInstanceExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &allSupportInstanceExtensionCount, instancePropertiesList.data());
    for(const auto &checkExt : checkExtensions){
        auto find = std::find_if(instancePropertiesList.begin(), instancePropertiesList.end(), [&checkExt](VkExtensionProperties &vkInstanceExtProp){
            if(strcmp(vkInstanceExtProp.extensionName , checkExt) == 0 ) return true;
            return false;
        });
        if(find!= instancePropertiesList.end())
            std::cout << "vulkan support instance extension:" << checkExt << std::endl;
        else{
            throw std::runtime_error(std::format("vulkan not support this instance extension:{}", checkExt));
        }
    };
}

void VulkanRenderer::getPhysicalDevice() {
    uint32_t physicalDeviceCount{0};
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    std::cout << "GPU count:" << physicalDeviceCount << std::endl;
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    assert(not physicalDevices.empty());
    for(auto &device: physicalDevices){
        if(checkDeviceSuitable(device)){ // Once we discover the graphics family
            mainDevice.physicalDevice = device;
            break;
        }
    }
}

bool VulkanRenderer::checkDeviceSuitable(const VkPhysicalDevice &device) const{
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device, &props);
    std::cout << "GPU name:" <<props.deviceName << std::endl;
    QueueFamilyIndices indices = getQueueFamilies(device);
    if(indices.isValid())
        return true;
    // assert checking
    checkDeviceExtensionSupport(deviceExtensions);
    auto swapDetails = getSwapChainDetails(device);
    assert(not swapDetails.formatList.empty());
    assert(not swapDetails.presentModeList.empty());
    return false;
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(const VkPhysicalDevice &device) const {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount,nullptr );
    std::cout << "VkPhysicalDevice support queue family count:" << queueFamilyCount  << std::endl;
    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount, queueFamilyList.data() );

    int i=0;
    for(auto &queueFamily : queueFamilyList){
        // has graphics queue family
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT and queueFamily.queueCount >0){
            indices.graphicsFamily = i; // current only focus the GRAPHICS_FAMILY, 用整形自己做indice
            //std::cout << "find graphics queue VK_QUEUE_GRAPHICS_BIT,queue count: " << queueFamily.queueCount << " ,and graphicsFamily indices:" << indices.graphicsFamily<< std::endl;
        }

        // check if queue family supports presentation . graphics queue also is a presentation queue.!
        // 这里直接这样检查，不用else if. 因为graphics queue 也是 presentation queue
        VkBool32 presentationSupport =false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surfaceKhr, &presentationSupport);
        if(queueFamily.queueCount> 0 && presentationSupport){
            indices.presentationFamily = i;
        }
        if(indices.isValid()){
            break;
        }

        i++;
    }
    return indices;
}

void VulkanRenderer::createLogicDevice() {
    auto queueFamilies = getQueueFamilies(mainDevice.physicalDevice);

    std::vector<VkDeviceQueueCreateInfo > queueInfos;
    std::set<int> queueFamilyIndices { queueFamilies.graphicsFamily, queueFamilies.presentationFamily}; // 其实是一个
    for(auto &familyIndex : queueFamilyIndices){
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        float queuePriorities[] = {1.0f}; // max is 1.0f
        queueCreateInfo.pQueuePriorities = queuePriorities;
        queueCreateInfo.queueCount = 1;// 当前family只用其中1个queue
        queueCreateInfo.queueFamilyIndex = familyIndex;
        queueInfos.emplace_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(mainDevice.physicalDevice, &features); // 非必要


    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueInfos.data();               // 注意这里可以挂多个VkQueueCreateInfo, 目前本质只挂一个GraphicsFamily中的一个Queue, 因为GraphicsFamily和Queue是一样的
    deviceCreateInfo.queueCreateInfoCount = queueInfos.size();
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;
    deviceCreateInfo.pEnabledFeatures =  &features;
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    auto result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
    if(result!=VK_SUCCESS) throw std::runtime_error{"create device error"};

    // create Queue
    vkGetDeviceQueue(mainDevice.logicalDevice, queueFamilies.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(mainDevice.logicalDevice, queueFamilies.presentationFamily, 0,&presentationQueue);
    std::cout <<"graphics queue:" <<graphicsQueue << " presentation queue:"<< presentationQueue << std::endl;

}

void VulkanRenderer::createSurface() {
    auto result = glfwCreateWindowSurface(instance, window, nullptr, &surfaceKhr);
    if(result!= VK_SUCCESS) {
        throw std::runtime_error{"ERROR create surface"};
    }
}


void VulkanRenderer::checkDeviceExtensionSupport(const std::vector<const char*> &checkExtensions) const {
    uint32_t propCount{0};
    vkEnumerateDeviceExtensionProperties(mainDevice.physicalDevice, nullptr, &propCount, nullptr);
    std::vector<VkExtensionProperties > propList(propCount);
    vkEnumerateDeviceExtensionProperties(mainDevice.physicalDevice, nullptr, &propCount, propList.data());
    for(const auto &checkExt : checkExtensions){
        auto find = std::find_if(propList.begin(), propList.end(), [&checkExt](VkExtensionProperties &vkInstanceExtProp){
            if(strcmp(vkInstanceExtProp.extensionName , checkExt) == 0 ) return true;
            return false;
        });
        if(find!= propList.end())
            std::cout << "vulkan support device extension:" << checkExt << std::endl;
        else{
            throw std::runtime_error(std::format("vulkan not support this device extension:{}", checkExt));
        }
    };
}
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &inputs) {
    for(const auto &[sFormat,sColorSpace] : inputs){
        bool cond1 = (sFormat == surfaceFormatChoose.format) or (sFormat == VK_FORMAT_B8G8R8A8_SNORM);
        bool cond2 = sColorSpace == surfaceFormatChoose.colorSpace;
        if(cond1 and cond2)
            return {sFormat, sColorSpace};
    }
    return surfaceFormatChoose;
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR> &inputs){
    for(const auto &pm : inputs)
        if(pm == presentModeChoose_primary) return pm;
    return presentModeChoose_secondary;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &input) const{
#undef max
    if(input.currentExtent.width != std::numeric_limits<uint32_t>::max() ){
        return input.currentExtent; // 一般情况下 根据input 得到的就是窗口大小
    }
    else {
        int width, height = 0;
        glfwGetFramebufferSize(window, &width, &height );
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }
}



SwapChainDetails VulkanRenderer::getSwapChainDetails(const VkPhysicalDevice  &device) const {
    auto ret = SwapChainDetails{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surfaceKhr, &ret.capabilities);
    uint32_t surfaceFormatCount{0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surfaceKhr, &surfaceFormatCount, nullptr);
    ret.formatList.resize(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surfaceKhr, &surfaceFormatCount, ret.formatList.data());

    uint32_t presentModeCount{0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surfaceKhr, &presentModeCount, nullptr);
    ret.presentModeList.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surfaceKhr, &presentModeCount, ret.presentModeList.data());
    return ret;
}

void VulkanRenderer::createSwapChain() {
#undef min
    auto [capabilities, formatList, presentModeList] = getSwapChainDetails(mainDevice.physicalDevice);
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
    createInfoKhr.surface = surfaceKhr;
    createInfoKhr.presentMode = surfacePresentationMode;
    createInfoKhr.minImageCount = minImageCount;
    createInfoKhr.imageExtent = extent;
    createInfoKhr.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfoKhr.imageArrayLayers = 1; // AI. 它定义了交换链图像的层级数量，这对于立体图像（例如3D）或者多重采样图像是非常有用的。
    createInfoKhr.preTransform = capabilities.currentTransform; //AI.  VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR 表示不对表面进行任何变换。这意味着图形将以其原始方向显示，不会进行旋转或镜像翻转。
    createInfoKhr.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfoKhr.clipped = VK_TRUE; // AI. 如果clipped设置为VK_TRUE，则表示在交换链图像的渲染中要裁剪掉在屏幕边界之外的像素，以提高性能。

    auto indices = getQueueFamilies(mainDevice.physicalDevice);
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
    auto result= vkCreateSwapchainKHR(mainDevice.logicalDevice, &createInfoKhr, nullptr, &swapChainKhr);
    if(result != VK_SUCCESS){
        throw std::runtime_error("Failed create Swap chain");
    }
    swapChainFormat = format;
    swapChainExtent = extent;

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapChainKhr, &swapChainImageCount, nullptr);
    assert(swapChainImageCount == 3);
    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapChainKhr, &swapChainImageCount, images.data());

    for(const auto &img : images) {
        SwapChainImage scImg{};
        scImg.image = img;
        scImg.imageView = createImageView(img, swapChainFormat, VK_IMAGE_ASPECT_COLOR_BIT );
        swapChainImages.emplace_back(scImg);
    }

}
VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const{
    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags; // (COLOR_BIT) VK_IMAGE_ASPECT_COLOR_BIT VK_IMAGE_ASPECT_DEPTH_BIT ...
    viewCreateInfo.subresourceRange.baseMipLevel = 0;// only look the level 0
    viewCreateInfo.subresourceRange.levelCount = 1; // number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;    // number of array levels to view

    VkImageView view{};
    if (auto ret = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &view ); ret!=VK_SUCCESS) {
        throw std::runtime_error{"ERROR create the image view"};
    }
    return view;
}


void VulkanRenderer::createPipeline(){
    simplePipeline.bindDevice = mainDevice.logicalDevice;
    simplePipeline.bindExtent = swapChainExtent;
    simplePipeline.bindRenderPass = simplePass.pass;
    simplePipeline.init();
}
void VulkanRenderer::createRenderpass(){
    simplePass.bindDevice = mainDevice.logicalDevice;
    simplePass.init();
}


