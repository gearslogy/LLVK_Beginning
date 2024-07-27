//
// Created by star on 4/29/2024.
//

#ifndef FRAMBUFFER_H
#define FRAMBUFFER_H

#include <vector>
#include <vulkan/vulkan.h>
#include "Utils.h"
struct Frambuffer {
    // REF OBJECT
    VkDevice bindDevice;
    VkRenderPass bindRenderPass;
    const std::vector<SwapChainImage> *bindSwapChainImages;
    VkImageView bindDepthImageView;
    const VkExtent2D *bindSwapChainExtent;
    // created object
    std::vector<VkFramebuffer> swapChainFramebuffers;
    void init();
    void cleanup();

};



#endif //FRAMBUFFER_H
