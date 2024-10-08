//
// Created by star on 3/27/2024.
//

#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <fstream>
#include <format>
#include <ranges>
#include "LLVK_SYS.hpp"

LLVK_NAMESPACE_BEGIN

// functions
namespace UT_Fn {
    constexpr auto xrange(int start,const Concept::is_range auto &container) {
        return std::views::iota(0, static_cast<int>(std::size(container)) );
    }
    constexpr auto xrange(const Concept::is_range auto &container) {
        return xrange(0, container);
    }
    constexpr auto xrange(auto start, auto end) {
        return std::views::iota(static_cast<int>(start), static_cast<int>(end) );
    }
    constexpr auto enumerate(Concept::is_range auto && container) {
        return std::views::zip(xrange(container), container);
    }
    constexpr auto cleanup_range_resources(Concept::is_range auto &&range) {
        for(auto &t : range) t.cleanup();
    }
    constexpr auto cleanup_resources(auto && ... r) {
        (r.cleanup(),...);
    }
    constexpr auto cleanup_shader_module(VkDevice device, const Concept::is_shader_module auto& ... module) {
        (vkDestroyShaderModule(device, module, nullptr),...);
    }
    constexpr auto cleanup_shader_module(VkDevice device, const Concept::is_range auto &container) {
        for (const auto &t : container)
            vkDestroyShaderModule(device, t, nullptr);
    }

    constexpr auto cleanup_descriptor_set_layout(VkDevice device, auto ... layout) {
        (vkDestroyDescriptorSetLayout(device, layout, nullptr),...);
    }
    constexpr auto cleanup_pipeline_layout(VkDevice device, auto ... layout) {
       ( vkDestroyPipelineLayout(device,layout,nullptr),... );
    }
    constexpr auto cleanup_pipeline(VkDevice device, auto ... pipeline) {
        (vkDestroyPipeline(device,pipeline,nullptr),...);
    }
    constexpr auto cleanup_sampler(VkDevice device, auto ... sampler) {
        (vkDestroySampler(device,sampler,nullptr),...);
    }
    constexpr auto cleanup_render_pass(VkDevice device, auto ... pass) {
        (vkDestroyRenderPass(device,pass,nullptr),...);
    }


    void invoke_and_check(const char *msg, auto func, auto && ... args) {
        if (func( std::forward<decltype(args)>(args)...  ) != VK_SUCCESS)
            throw std::runtime_error{msg};
    }

}



struct QueueFamilyIndices{
    int graphicsFamily{-1};
    int presentationFamily{-1};

    [[nodiscard]] bool isValid() const{
        return graphicsFamily >=0;
    }
};

inline QueueFamilyIndices getQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device)  {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount,nullptr );
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
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


struct SwapChainDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formatList; //image formats: RGBA and size of each color.  R8G8A8...
    std::vector<VkPresentModeKHR> presentModeList;
};

struct SwapChainImage{
    VkImage image;
    VkImageView imageView;
};

inline std::vector<char> readSpvFile(const std::string &path) {
    std::vector<char> ret;
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if(! in.is_open()) {
        throw std::runtime_error{std::format("can not open file: {}" , path)};
    }
    size_t fileSize = in.tellg();
    ret.resize(fileSize);
    in.seekg(0);
    in.read(ret.data(), fileSize);
    in.close();
    return ret;
}

inline VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    VkShaderModule ret{};
    if(auto succ = vkCreateShaderModule(device, &createInfo, nullptr, &ret) ; succ!=VK_SUCCESS) {
        throw std::runtime_error{"error"};
    }
    return ret;
}

/*
VkMemoryRequirements memRequirements;
vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
VkMemoryAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
*/
inline uint32_t findMemoryType(VkPhysicalDevice device,uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    for(int i=0;i<memProperties.memoryTypeCount;i++) {
        bool condition1 = typeFilter & (1 << i);
        bool condition2 = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if(condition1 and condition2) {
            return i;
        }
    }
    throw std::runtime_error{"can not get memory type"};
}

LLVK_NAMESPACE_END
