//
// Created by star on 5/3/2024.
//

#include "Device.h"
#include <cassert>
#include <iostream>
#include  "LLVK_Utils.hpp"
#include "Swapchain.h"
#include <set>
LLVK_NAMESPACE_BEGIN


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
            auto maxPushConstantSize = getMaxPushConstantsSize(physicalDevice) ;
            std::cout << "[[Device::getPhysicalDevice]]: MaxPushConstantsSize:" << maxPushConstantSize<< ", The number of floats that can fit:"<< maxPushConstantSize / sizeof(float) << std::endl;
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU device!");
    }

}
void Device::createLogicDevice() {
    std::vector<VkDeviceQueueCreateInfo > queueInfos;

    std::set unionQueueFamilyIndices{
        queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentationFamily, queueFamilyIndices.computeFamily, // 其实是一个
        queueFamilyIndices.transferFamily};  // single

    for(auto &familyIndex : unionQueueFamilyIndices){
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        float queuePriorities[] = {1.0f}; // max is 1.0f
        queueCreateInfo.pQueuePriorities = queuePriorities;
        queueCreateInfo.queueCount = 1;// 当前family只用其中1个queue
        queueCreateInfo.queueFamilyIndex = familyIndex;
        queueInfos.emplace_back(queueCreateInfo);
    }

    /*
    // disable this features. use features2
    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &features); // 非必要
    if (!features.geometryShader) {
        throw std::runtime_error("geometry shader not supported!");
    }

    */


    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueInfos.data();               // 注意这里可以挂多个VkQueueCreateInfo, 目前本质只挂一个GraphicsFamily中的一个Queue, 因为GraphicsFamily和Queue是一样的
    deviceCreateInfo.queueCreateInfoCount = queueInfos.size();
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;
    deviceCreateInfo.pEnabledFeatures =  nullptr; // use next VkPhysicalDeviceFeatures2
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    {
        // Feature chain setup
        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
        indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

        VkPhysicalDeviceMultiviewFeaturesKHR multiviewFeatures{};
        multiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;

        VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeatures{};
        fragmentShadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        // Link the feature chain
        deviceFeatures2.pNext = &meshShaderFeatures;
        meshShaderFeatures.pNext = &multiviewFeatures;
        multiviewFeatures.pNext = &fragmentShadingRateFeatures;
        fragmentShadingRateFeatures.pNext = &indexingFeatures;

        // Query supported features
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        // Check bindless support
        bool bindlessSupported = indexingFeatures.descriptorBindingPartiallyBound &&
                                indexingFeatures.runtimeDescriptorArray;

        // Check mesh shader support
        bool meshShaderSupported = meshShaderFeatures.meshShader && meshShaderFeatures.taskShader;

        // Check multiview support for mesh shaders
        bool multiviewSupported = multiviewFeatures.multiview &&
                                 meshShaderFeatures.multiviewMeshShader;

        // Check fragment shading rate support
        bool fragmentShadingRateSupported = fragmentShadingRateFeatures.primitiveFragmentShadingRate &&
                                          meshShaderFeatures.primitiveFragmentShadingRateMeshShader;

        // Enable features if supported
        if (meshShaderSupported) {
            meshShaderFeatures.meshShader = VK_TRUE;
            meshShaderFeatures.taskShader = VK_TRUE;
        }

        if (multiviewSupported && meshShaderSupported && meshShaderFeatures.multiviewMeshShader) {
            multiviewFeatures.multiview = VK_TRUE;
            meshShaderFeatures.multiviewMeshShader = VK_TRUE;
        }

        if (fragmentShadingRateSupported && meshShaderSupported &&
            meshShaderFeatures.primitiveFragmentShadingRateMeshShader) {
            fragmentShadingRateFeatures.primitiveFragmentShadingRate = VK_TRUE;
            meshShaderFeatures.primitiveFragmentShadingRateMeshShader = VK_TRUE;
        }

        deviceCreateInfo.pNext = &deviceFeatures2;

        // Log supported features
        if (bindlessSupported) {
            std::cout << "[[Device::getPhysicalDevice]]: Bindless supported" << std::endl;
        }
        if (deviceFeatures2.features.geometryShader) {
            std::cout << "[[Device::getPhysicalDevice]]: Geometry shader supported" << std::endl;
        }
        if (meshShaderSupported) {
            std::cout << "[[Device::getPhysicalDevice]]: Mesh shader supported" << std::endl;
        }
        if (multiviewSupported) {
            std::cout << "[[Device::getPhysicalDevice]]: Multiview supported" << std::endl;
        }
        if (fragmentShadingRateSupported) {
            std::cout << "[[Device::getPhysicalDevice]]: Primitive fragment shading rate supported" << std::endl;
        }
    }

    auto result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
    if(result!=VK_SUCCESS) throw std::runtime_error{"create device error"};

    // create Queue
    vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, queueFamilyIndices.presentationFamily, 0,&presentationQueue);
    vkGetDeviceQueue(logicalDevice, queueFamilyIndices.computeFamily, 0, &computeQueue);
    vkGetDeviceQueue(logicalDevice, queueFamilyIndices.transferFamily, 0, &transferQueue);
    std::string info = std::format("graphicsQueue: {:#x}, presentationQueue: {:#x}, computeQueue: {:#x}, transferQueue:{:#x}",
        reinterpret_cast<uint64_t>(graphicsQueue),
        reinterpret_cast<uint64_t>(presentationQueue),
        reinterpret_cast<uint64_t>(computeQueue),
        reinterpret_cast<uint64_t>(transferQueue)
        );
    std::cout << info << std::endl;
}


bool Device::checkDeviceSuitable(const VkPhysicalDevice &device) {
    bool condition0{false};
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device, &props);
    std::cout << "[[Device::checkDeviceSuitable]]:" <<" GPU name:" <<props.deviceName << std::endl;
    std::cout << "[[Device::checkDeviceSuitable]]:"<< " GPU maxMemoryAllocationCount:" << props.limits.maxMemoryAllocationCount << std::endl;
    std::cout << "[[Device::checkDeviceSuitable]]:"<<" GPU maxUniformBufferRange:"<<props.limits.maxUniformBufferRange << " bytes" << std::endl;
    std::cout << "[[Device::checkDeviceSuitable]]:"<<" GPU maxStorageBufferRange:"<<static_cast<float>(props.limits.maxStorageBufferRange) / 1024 / 1024 / 1024 << " GB" << std::endl;
    std::cout << "[[Device::checkDeviceSuitable]]:"<<" GPU maxStorageBufferRange support vec4 count:"<<static_cast<float>(props.limits.maxStorageBufferRange) / sizeof(float)*4 << " vec4" << std::endl; // sizeof(vec4) = 16bytes

    queueFamilyIndices = getQueueFamilies(bindSurface,device);
    condition0 = queueFamilyIndices.isValid();
    auto swapDetails = Swapchain::getSwapChainDetails(bindSurface,device);
    auto condition1 = not swapDetails.formatList.empty();
    auto condition2 = not swapDetails.presentModeList.empty();
    auto condition3 = checkDeviceExtensionSupport(device,deviceExtensions);
    auto allCond = condition0 && condition1 && condition2 && condition3;
    std::cout << "[[Device::checkDeviceSuitable]]:"<< " CHECK RESULT:"<< allCond  << std::endl; // sizeof(vec4) = 16bytes
    return allCond;
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
            std::cout << "[[Device::checkDeviceExtensionSupport]]: vulkan support device extension:" << checkExt << std::endl;
            accumCheck +=1;
        }
        else{
            std::cout << "vulkan do not support device extension:" << checkExt << std::endl;
        }
    };
    return accumCheck == checkExtensions.size();
}

uint32_t Device::getMaxPushConstantsSize(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    uint32_t maxPushConstantsSize = deviceProperties.limits.maxPushConstantsSize;
    return maxPushConstantsSize;
}
LLVK_NAMESPACE_END