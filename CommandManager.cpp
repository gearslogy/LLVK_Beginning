//
// Created by star on 4/28/2024.
//

#include "CommandManager.h"
#include "Utils.h"
#include <iostream>
void CommandManager::cleanup() {
    vkDestroyCommandPool(bindLogicDevice, graphicsCommandPool, nullptr);
}
void CommandManager::init() {
    // Get indices of queue families from device
    QueueFamilyIndices queueFamilyIndices = getQueueFamilies(bindSurface, bindPhysicalDevice);
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;	// Queue Family type that buffers from this command pool will use
    auto result = vkCreateCommandPool(bindLogicDevice, &poolInfo, nullptr, &graphicsCommandPool);
    if (result != VK_SUCCESS){
        throw std::runtime_error("Failed to create a Command Pool!");
    }
    // create command buffers
    createCommandBuffers();
}

void CommandManager::createCommandBuffers() {

    // Resize command buffer count to have one for each framebu4ffer
    swapChainCommandBuffers.resize(bindSwapChainFramebuffers.size());
    std::cout << __FUNCTION__ << " create swapChainCommandBuffers size:" << swapChainCommandBuffers.size() << std::endl;
    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = graphicsCommandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(swapChainCommandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    VkResult result = vkAllocateCommandBuffers(bindLogicDevice, &cbAllocInfo, swapChainCommandBuffers.data());
    if (result != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate Command Buffers!");
    }
}
