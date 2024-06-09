//
// Created by star on 5/3/2024.
//

#include "Device.h"
#include <cassert>
#include <iostream>
#include  "Utils.h"
#include "Swapchain.h"
#include <set>
void Device::init() {
    getPhysicalDevice();
    createLogicDevice();
}
void Device::cleanup() {
    vkDestroyDevice(logicalDevice, nullptr);
}

void Device::getPhysicalDevice() {
    uint32_t physicalDeviceCount{0};
    vkEnumeratePhysicalDevices(bindInstance, &physicalDeviceCount, nullptr);
    std::cout << "GPU count:" << physicalDeviceCount << std::endl;
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(bindInstance, &physicalDeviceCount, physicalDevices.data());
    assert(not physicalDevices.empty());
    for(auto &device: physicalDevices){
        if(checkDeviceSuitable(device)){ // Once we discover the graphics family
            physicalDevice = device;
            break;
        }
    }
    assert(physicalDevice!=VK_NULL_HANDLE);
}
void Device::createLogicDevice() {
    auto queueFamilies = getQueueFamilies(bindSurface,physicalDevice);

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
    vkGetPhysicalDeviceFeatures(physicalDevice, &features); // 非必要

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueInfos.data();               // 注意这里可以挂多个VkQueueCreateInfo, 目前本质只挂一个GraphicsFamily中的一个Queue, 因为GraphicsFamily和Queue是一样的
    deviceCreateInfo.queueCreateInfoCount = queueInfos.size();
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;
    deviceCreateInfo.pEnabledFeatures =  &features;
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    auto result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
    if(result!=VK_SUCCESS) throw std::runtime_error{"create device error"};

    // create Queue
    vkGetDeviceQueue(logicalDevice, queueFamilies.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, queueFamilies.presentationFamily, 0,&presentationQueue);
    std::cout <<"graphics queue:" <<graphicsQueue << " presentation queue:"<< presentationQueue << std::endl;
}


bool Device::checkDeviceSuitable(const VkPhysicalDevice &device) const{
    bool condition0{false};
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device, &props);
    std::cout << "GPU name:" <<props.deviceName << std::endl;
    std::cout << "GPU maxMemoryAllocationCount:" << props.limits.maxMemoryAllocationCount << std::endl;
    QueueFamilyIndices indices = getQueueFamilies(bindSurface,device);
    condition0 = indices.isValid();
    // assert checking

    auto swapDetails = Swapchain::getSwapChainDetails(bindSurface,device);
    auto condition1 = not swapDetails.formatList.empty();
    auto condition2 = not swapDetails.presentModeList.empty();
    auto condition3 = checkDeviceExtensionSupport(device,deviceExtensions);
    return condition0 and condition1 and condition2 and condition3;
}
bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char *> &checkExtensions) {
    uint32_t propCount{0};
    vkEnumerateDeviceExtensionProperties(device, nullptr, &propCount, nullptr);
    std::vector<VkExtensionProperties > propList(propCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &propCount, propList.data());

    int accumCheck{0};
    for(const auto &checkExt : checkExtensions){
        auto find = std::find_if(propList.begin(), propList.end(), [&checkExt](VkExtensionProperties &vkInstanceExtProp){
            if(strcmp(vkInstanceExtProp.extensionName , checkExt) == 0 ) return true;
            return false;
        });
        if(find != propList.end()){
            std::cout << "vulkan support device extension:" << checkExt << std::endl;
            accumCheck +=1;
        }
        else{
            std::cout << "vulkan do not support device extension:" << checkExt << std::endl;
        }
    };
    return accumCheck == checkExtensions.size();
}
