//
// Created by star on 4/28/2024.
//

#include "CommandManager.h"
#include "Utils.h"
#include <iostream>
void CommandManager::cleanup() {
    vkDestroyCommandPool(bindLogicDevice, graphicsCommandPool, nullptr);
}


void CommandManager::createGraphicsCommandPool() {
    QueueFamilyIndices queueFamilyIndices = getQueueFamilies(bindSurface, bindPhysicalDevice);
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;	// Queue Family type that buffers from this command pool will use
    auto result = vkCreateCommandPool(bindLogicDevice, &poolInfo, nullptr, &graphicsCommandPool);
    if (result != VK_SUCCESS){
        throw std::runtime_error("Failed to create a Command Pool!");
    }
}


void CommandManager::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    std::cout << __FUNCTION__ << " create swapChainCommandBuffers MAX_FRAMES_IN_FLIGHT size:" << commandBuffers.size() << std::endl;
    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = graphicsCommandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    VkResult result = vkAllocateCommandBuffers(bindLogicDevice, &cbAllocInfo, commandBuffers.data());
    if (result != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate Command Buffers!");
    }
}

void CommandManager::recordCommand(VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
    VkClearValue clearValues[1];
    clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.framebuffer = (*bindSwapChainFramebuffers)[imageIndex];
    renderPassBeginInfo.renderPass = bindRenderPass;
    renderPassBeginInfo.renderArea.extent = *bindSwapChainExtent;
    renderPassBeginInfo.renderArea.offset = {0,0};
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.clearValueCount = 1;
    auto result = vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo);
    if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS ,bindPipeline);
        VkViewport viewport{};
        viewport.width = static_cast<float>(bindSwapChainExtent->width);
        viewport.height = static_cast<float>(bindSwapChainExtent->height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
        VkRect2D scissor{{0,0}, *bindSwapChainExtent};
        vkCmdSetScissor(cmdBuffer,0, 1, &scissor);
        /* draw non-indexed buffer
        vkCmdBindVertexBuffers(cmdBuffer,
            bindVertexBuffers.firstBinding,
            bindVertexBuffers.bindingCount,
            bindVertexBuffers.vertexBuffers.data(),
            bindVertexBuffers.offsets.data());
        vkCmdDraw(cmdBuffer,bindVertexBuffers.vertexCount, 1,0, 0);
        */
        // draw index buffer
        vkCmdBindVertexBuffers(cmdBuffer,
                           bindVertexBuffers.firstBinding,
                           bindVertexBuffers.bindingCount,
                           bindVertexBuffers.vertexBuffers.data(),
                           bindVertexBuffers.offsets.data());
        vkCmdBindIndexBuffer(cmdBuffer, bindIndexBuffer.indexBuffer, bindIndexBuffer.offset,bindIndexBuffer.indexType);
        vkCmdDrawIndexed(cmdBuffer,bindIndexBuffer.indexCount,1,0,0,0 );

    vkCmdEndRenderPass(cmdBuffer);
    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}







