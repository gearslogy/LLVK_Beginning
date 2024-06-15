//
// Created by star on 4/29/2024.
//

#include "Frambuffer.h"
#include <array>
#include <iostream>
void Frambuffer::cleanup() {
    for (auto buffer: swapChainFramebuffers) {
        vkDestroyFramebuffer(bindDevice, buffer, nullptr);
    }
}
void Frambuffer::init() {
    // Resize framebuffer count to equal swap chain image count
    swapChainFramebuffers.resize(bindSwapChainImages->size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
    {
        std::array attachments = {
            (*bindSwapChainImages)[i].imageView,
            bindDepthImageView
        };
        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = bindRenderPass;										// Render Pass layout the Framebuffer will be used with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();							// List of attachments (1:1 with Render Pass)
        framebufferCreateInfo.width = bindSwapChainExtent->width;								// Framebuffer width
        framebufferCreateInfo.height = bindSwapChainExtent->height;								// Framebuffer height
        framebufferCreateInfo.layers = 1;													// Framebuffer layers

        auto result = vkCreateFramebuffer(bindDevice, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Framebuffer!");
        }
    }
}

