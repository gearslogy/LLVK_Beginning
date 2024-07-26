//
// Created by star on 3/27/2024.
//

#ifndef CP_02_UTILS_H
#define CP_02_UTILS_H


#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <fstream>
#include <format>
#include <ranges>
constexpr bool ALWAYS_TRUE = true;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

namespace LLVK {
    namespace Concept {
        template<typename T>
        concept is_range = requires(T var){
            var.size();
            var.data();
        };
    }

    constexpr auto xrange(int start,const Concept::is_range auto &container) {
        return std::views::iota(0, static_cast<int>(std::size(container)) );
    }

}

/*
namespace ranges {
    template <typename Rng, template <typename...> class Cont = std::vector>
    auto to(Rng&& rng) {
        using value_type = std::ranges::range_value_t<Rng>;
        return Cont<value_type>(std::ranges::begin(rng), std::ranges::end(rng));
    }
}*/

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


#endif //CP_02_UTILS_H
