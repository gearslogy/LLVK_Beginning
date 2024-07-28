//
// Created by star on 4/28/2024.
//

#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <vector>
#include "PushConstant.hpp"
#include <iostream>
LLVK_NAMESPACE_BEGIN
struct FnCommand {
    // single time command
    static VkCommandBuffer beginSingleTimeCommand(VkDevice device, VkCommandPool pool);

    static void endSingleTimeCommand(VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer cmdBuf);

    static VkCommandPool createCommandPool(VkDevice device, uint32_t queueFamilyIndex,
                                           const VkCommandPoolCreateInfo *poolCreateInfo = nullptr);


    // when command begin
    struct RenderCommandBeginInfo {
        VkCommandBufferBeginInfo commandBufferBeginInfo;
        VkRenderPassBeginInfo renderPassBeginInfo;
    };
    static RenderCommandBeginInfo createCommandBufferBeginInfo(const VkFramebuffer &framebuffer,
                                                         const VkRenderPass &renderpass,
                                                         const VkExtent2D *swapChainExtent,
                                                         const std::vector<VkClearValue> &clearValues);
    //vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    static VkViewport viewport(VkCommandBuffer cmdBuffer, auto width, auto height) {
        VkViewport viewport{};
        viewport.width = static_cast<float>(width);
        viewport.height = static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewport.x = 0;
        viewport.y = 0;
        return viewport;
    }
    //vkCmdSetScissor(cmdBuffer,0, 1, &rect2D);
    static VkRect2D scissor(VkCommandBuffer cmdBuffer,auto width,auto height){
        VkRect2D rect2D {};
        rect2D.extent.width = static_cast<int32_t>(width);
        rect2D.extent.height = static_cast<int32_t>(height);
        rect2D.offset.x = 0;
        rect2D.offset.y = 0;
        return rect2D;
    }



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


LLVK_NAMESPACE_END
#endif //COMMANDBUFFER_H
