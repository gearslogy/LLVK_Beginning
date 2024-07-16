//
// Created by star on 4/28/2024.
//

#include "CommandManager.h"
#include "Utils.h"
#include <iostream>
#include <array>
#include "GeoVertexDescriptions.h"
VkCommandBuffer FnCommand::beginSingleTimeCommand(VkDevice device, VkCommandPool pool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = pool;
    VkCommandBuffer ret{};
    if(vkAllocateCommandBuffers(device, &allocInfo, &ret)!=VK_SUCCESS ) {
        throw std::runtime_error{"can not allocate single time command buffer "};
    }
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if(vkBeginCommandBuffer(ret, &beginInfo) != VK_SUCCESS) throw std::runtime_error{"begin copy command buffer error"};
    return ret;
}

void FnCommand::endSingleTimeCommand(VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer cmdBuf) {
    if(vkEndCommandBuffer(cmdBuf) != VK_SUCCESS) throw std::runtime_error{"begin copy command buffer error"};
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;
    vkQueueSubmit(queue,1, &submitInfo,VK_NULL_HANDLE );
    vkQueueWaitIdle(queue); //vkDeviceWaitIdle(device);
    vkFreeCommandBuffers(device, pool, 1, &cmdBuf);
}

VkCommandPool FnCommand::createCommandPool(VkDevice device, uint32_t queueFamilyIndex, const VkCommandPoolCreateInfo *poolCreateInfo) {
    VkCommandPool ret{};
    if(poolCreateInfo != nullptr) {
        if (vkCreateCommandPool(device, poolCreateInfo, nullptr, &ret) != VK_SUCCESS)
            throw std::runtime_error("Failed to create a Command Pool!");
    }
    else { // use default
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndex;	// Queue Family type that buffers from this command pool will use
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &ret) != VK_SUCCESS)
            throw std::runtime_error("Failed to create a Command Pool!");
    }
    return ret;
}

FnCommand::RenderCommandBeginInfo FnCommand::createCommandBufferBeginInfo(const VkFramebuffer &framebuffer,
                                                                    const VkRenderPass &renderpass,
                                                                    const VkExtent2D *swapChainExtent,
                                                                    const std::vector<VkClearValue> &clearValues) {

    // cmd begin
    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    // pass begin
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderPass = renderpass;
    renderPassBeginInfo.renderArea.extent = *swapChainExtent;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.pClearValues = clearValues.data();
    renderPassBeginInfo.clearValueCount = clearValues.size();
    return {cmdBufBeginInfo, renderPassBeginInfo};
}










