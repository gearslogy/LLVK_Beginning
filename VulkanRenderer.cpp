
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
    simpleSwapchain.cleanup();
    vkDestroySurfaceKHR(instance,surfaceKhr, nullptr);
    vkDestroyDevice(mainDevice.logicalDevice, nullptr);
    if(enableValidation){
        simpleDebug.cleanup();
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
    std::vector<const char *> extensionNamesList;
    for(int i=0;i<glfwExtPropsCount;i++){
        extensionNamesList.emplace_back(glfwExtensionNames[i]);
    }
    if(enableValidation) {
        //extensionNamesList.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME); // VK_EXT_debug_report, this is old method(v1)
        extensionNamesList.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);  // we will use here
    }

    checkInstanceExtensionSupport(extensionNamesList);

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = DebugV2::CustomDebug::getCreateInfo();
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensionNamesList.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensionNamesList.data();
    instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
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
    simpleDebug.bindInstance = instance;
    simpleDebug.init();
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
        if(find!= instancePropertiesList.end()) {
            //std::cout << "vulkan support instance extension:" << checkExt << std::endl;
        }
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
    QueueFamilyIndices indices = getQueueFamilies(surfaceKhr,device);
    if(indices.isValid())
        return true;
    // assert checking
    checkDeviceExtensionSupport(deviceExtensions);
    auto swapDetails = Swapchain::getSwapChainDetails(surfaceKhr,device);
    assert(not swapDetails.formatList.empty());
    assert(not swapDetails.presentModeList.empty());
    return false;
}



void VulkanRenderer::createLogicDevice() {
    auto queueFamilies = getQueueFamilies(surfaceKhr,mainDevice.physicalDevice);

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


void VulkanRenderer::createSwapChain() {
    simpleSwapchain.bindLogicDevice = mainDevice.logicalDevice;
    simpleSwapchain.bindPhyiscalDevice = mainDevice.physicalDevice;
    simpleSwapchain.bindSurface = surfaceKhr;
    simpleSwapchain.bindWindow = window;
    simpleSwapchain.init();
}


void VulkanRenderer::createPipeline(){
    simplePipeline.bindDevice = mainDevice.logicalDevice;
    simplePipeline.bindExtent = simpleSwapchain.swapChainExtent;
    simplePipeline.bindRenderPass = simplePass.pass;
    simplePipeline.init();
}
void VulkanRenderer::createRenderpass(){
    simplePass.bindDevice = mainDevice.logicalDevice;
    simplePass.init();
}


