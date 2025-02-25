//
// Created by star on 4/7/2024.
//

#ifndef CP_02_VULKANVALIDATION_H
#define CP_02_VULKANVALIDATION_H


#include <vulkan/vulkan.h>
#include <iostream>
#include <format>
#include <glm/glm.hpp>
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
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugFunction(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData){
        //std::cerr << "*****validation layer: " << pCallbackData->pMessage << std::endl;
        const char* severityStr;
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                severityStr = "VERBOSE";
            break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                severityStr = "INFO";
            break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                severityStr = "WARNING";
            break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                severityStr = "ERROR";
            break;
            default:
                severityStr = "UNKNOWN";
        }
        printf("[%s] %s\n", severityStr, pCallbackData->pMessage);
        return VK_FALSE;
    }
    inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
    struct CustomDebug {
        VkInstance bindInstance{}; // REF OBJECT
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        VkDebugUtilsMessengerEXT debugMessenger{};
        // rely on the VkInstance
        void init() {
            createInfo = getCreateInfo();
            if (CreateDebugUtilsMessengerEXT(bindInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
                throw std::runtime_error("failed to set up debug messenger!");
        }
        void cleanup() {
            DestroyDebugUtilsMessengerEXT(bindInstance, debugMessenger, nullptr);
        }
        static VkDebugUtilsMessengerCreateInfoEXT getCreateInfo() {
            VkDebugUtilsMessengerCreateInfoEXT ret{};
            ret.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            ret.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ;//| VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT;
            ret.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            ret.pfnUserCallback = debugFunction;
            return ret;
        }
    }; // end of CustomDebug

}

namespace DebugV2 {
    struct CommandLabel {
        inline static void init(VkInstance instance) {
            vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
            vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
            vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
        }
        //cmdBeginLabel(drawCmdBuffers[i], "Subpass 1: Deferred composition", { 0.0f, 0.5f, 1.0f, 1.0f });
        inline static void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color)
        {
            if (!vkCmdBeginDebugUtilsLabelEXT) {
                return;
            }
            VkDebugUtilsLabelEXT labelInfo{};
            labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            labelInfo.pLabelName = caption.c_str();
            memcpy(labelInfo.color, &color[0], sizeof(float) * 4);
            vkCmdBeginDebugUtilsLabelEXT(cmdbuffer, &labelInfo);
        }
        //cmdEndLabel(drawCmdBuffers[i]);
        inline static void cmdEndLabel(VkCommandBuffer cmdbuffer)
        {
            if (!vkCmdEndDebugUtilsLabelEXT)
                return;
            vkCmdEndDebugUtilsLabelEXT(cmdbuffer);
        }
    private:
        inline static PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
        inline static PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
        inline static PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr };
    };
}


#endif //CP_02_VULKANVALIDATION_H
