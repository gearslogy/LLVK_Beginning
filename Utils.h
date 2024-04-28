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
struct QueueFamilyIndices{
    int graphicsFamily{-1};
    int presentationFamily{-1};

    [[nodiscard]] bool isValid() const{
        return graphicsFamily >=0;
    }
};


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




#endif //CP_02_UTILS_H
