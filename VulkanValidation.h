//
// Created by star on 4/7/2024.
//

#ifndef CP_02_VULKANVALIDATION_H
#define CP_02_VULKANVALIDATION_H


#include <vulkan/vulkan.h>
#include <iostream>
#include <format>
namespace DebugV1 {
    /*
typedef enum VkDebugReportFlagBitsEXT {
    VK_DEBUG_REPORT_INFORMATION_BIT_EXT = 0x00000001,
    VK_DEBUG_REPORT_WARNING_BIT_EXT = 0x00000002,
    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT = 0x00000004,
    VK_DEBUG_REPORT_ERROR_BIT_EXT = 0x00000008,
    VK_DEBUG_REPORT_DEBUG_BIT_EXT = 0x00000010,
    VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
} VkDebugReportFlagBitsEXT;*/

inline VKAPI_ATTR VkBool32 VKAPI_CALL debugFunction(
        VkDebugReportFlagsEXT flags,                // Type of error
        VkDebugReportObjectTypeEXT objType,            // Type of object causing error
        uint64_t obj,                                // ID of object
        size_t location,
        int32_t code,
        const char *layerPrefix,
        const char *message,                        // Validation Information
        void *userData) {


    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        std::cout << std::format("[VK_DEBUG_REPORT] {}: [\"{}\"] Code {}: {}", "ERROR", layerPrefix, code, message)
                  << std::endl;
        return VK_TRUE;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        std::cout << std::format("[VK_DEBUG_REPORT] {}: [\"{}\"] Code {}: {}", "WARNING", layerPrefix, code, message)
                  << std::endl;
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        std::cout
                << std::format("[VK_DEBUG_REPORT] {}: [\"{}\"] Code {}: {}", "INFORMATION", layerPrefix, code, message)
                << std::endl;
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        std::cout
                << std::format("[VK_DEBUG_REPORT] {}: [\"{}\"] Code {}: {}", "PERFORMANCE", layerPrefix, code, message)
                << std::endl;
    } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        std::cout << std::format("[VK_DEBUG_REPORT] {}: [\"{}\"] Code {}: {}", "DEBUG", layerPrefix, code, message)
                  << std::endl;
    } else {
        return VK_FALSE;
    }
    return VK_FALSE;
}


inline VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugReportCallbackEXT *pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    std::cout << "FUNC----------->" << func << std::endl;
    if (func) {
        std::cout << "PFN_vkCreateDebugReportCallbackEXT success" << std::endl;
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

inline void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback,
                                          const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr)
        func(instance, callback, pAllocator);
}
}


namespace DebugV2 {

}





#endif //CP_02_VULKANVALIDATION_H
