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

    inline static VkCommandBufferBeginInfo commandBufferBeginInfo() {
        // cmd begin
        VkCommandBufferBeginInfo cmdBufBeginInfo{};
        cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        return cmdBufBeginInfo;
    }

    inline static VkRenderPassBeginInfo renderPassBeginInfo(const VkFramebuffer &framebuffer,
                                                            const VkRenderPass &renderpass,
                                                            const VkExtent2D extent,
                                                            std::span<const VkClearValue> clearValues) {
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.framebuffer = framebuffer;
        renderPassBeginInfo.renderPass = renderpass;
        renderPassBeginInfo.renderArea.extent = extent;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.pClearValues = clearValues.data();
        renderPassBeginInfo.clearValueCount = clearValues.size();
        return renderPassBeginInfo;
    }



    //vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    static VkViewport viewport( auto width, auto height) {
        VkViewport viewport{};
        viewport.width = static_cast<float>(width);
        viewport.height = static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewport.x = 0;
        viewport.y = 0;
        return viewport;
    }
    static VkViewport viewport(auto width, auto height, auto x, auto y) {
        auto ret = viewport(width, height);
        ret.x = x;
        ret.y = y;
        return ret;
    }

    //vkCmdSetScissor(cmdBuffer,0, 1, &rect2D);
    static VkRect2D scissor(auto width,auto height){
        VkRect2D rect2D {};
        rect2D.extent.width = static_cast<int32_t>(width);
        rect2D.extent.height = static_cast<int32_t>(height);
        rect2D.offset.x = 0;
        rect2D.offset.y = 0;
        return rect2D;
    }
    static VkRect2D scissor(auto width,auto height, auto offsetX, auto offsetY){
        VkRect2D rect2D = scissor(width,height);
        rect2D.offset.x = offsetX;
        rect2D.offset.y = offsetY;
        return rect2D;
    }

    
};




LLVK_NAMESPACE_END
#endif //COMMANDBUFFER_H
