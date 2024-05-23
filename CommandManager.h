//
// Created by star on 4/28/2024.
//

#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H
#include <vulkan/vulkan.h>
#include <vector>

struct FnCommand {
    // single time command
    static VkCommandBuffer beginSingleTimeCommand(VkDevice device, VkCommandPool pool);
    static void endSingleTimeCommand(VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer cmdBuf);
    static VkCommandPool createCommandPool(VkDevice device,uint32_t queueFamilyIndex, const VkCommandPoolCreateInfo *poolCreateInfo = nullptr);
};


/* struct for building command:vkCmdBindVertexBuffers
VkBuffer vertexBuffers[] = {};
VkDeviceSize offsets[] = {0}; // 0从每个buffer的起始位置开始读取
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
 */
struct CmdBindVertexBuffers {
    std::vector<VkBuffer>     vertexBuffers;
    std::vector<VkDeviceSize> offsets;
    uint32_t firstBinding;
    uint32_t bindingCount;
    size_t vertexCount; // only use vkCmdDraw
};
struct CmdBindIndexBuffer {
    VkBuffer indexBuffer;
    VkDeviceSize offset;
    VkIndexType indexType;
    size_t indexCount;  // only use vkCmdDrawIndexed
};

struct CommandManager {
    VkDevice bindLogicDevice{};
    VkPhysicalDevice bindPhysicalDevice{};
    VkSurfaceKHR bindSurface{};
    const std::vector<VkFramebuffer> *bindSwapChainFramebuffers;
    const VkExtent2D *bindSwapChainExtent;
    VkRenderPass bindRenderPass;
    VkPipeline bindPipeline; // which pipeline to rendering
    VkPipelineLayout bindPipeLineLayout;
    CmdBindVertexBuffers bindVertexBuffers;
    CmdBindIndexBuffer bindIndexBuffer;
    const std::vector<VkDescriptorSet> *bindDescriptorSets;
    const int *bindCurrentFrame;
    // created
    VkCommandPool graphicsCommandPool{};
    std::vector<VkCommandBuffer> commandBuffers;// Resize command buffer count to have one for each framebuffer
    void cleanup();
    void recordCommand(VkCommandBuffer cmdBuffer, uint32_t imageIndex);

    void createGraphicsCommandPool();
    void createCommandBuffers();

};



#endif //COMMANDBUFFER_H
