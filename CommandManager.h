//
// Created by star on 4/28/2024.
//

#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H
#include <vulkan/vulkan.h>
#include <vector>

struct CommandManager {
    VkDevice bindLogicDevice{};
    VkPhysicalDevice bindPhysicalDevice{};
    VkSurfaceKHR bindSurface{};
    std::vector<VkFramebuffer> bindSwapChainFramebuffers;
    // created
    VkCommandPool graphicsCommandPool{};
    std::vector<VkCommandBuffer> swapChainCommandBuffers;// Resize command buffer count to have one for each framebuffer
    void init();
    void cleanup();
private:
    void createCommandBuffers();

};



#endif //COMMANDBUFFER_H
