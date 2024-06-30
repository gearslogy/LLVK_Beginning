//
// Created by star on 4/25/2024.
//

#ifndef PIPELINE_H
#define PIPELINE_H
#include <vulkan/vulkan.h>
#include <array>
#include "PushConstant.h"
struct Pipeline {
    VkDevice bindDevice{};   // REF object
    VkExtent2D bindExtent{}; // REF object
    VkRenderPass bindRenderPass{}; // REF object
    std::array<VkDescriptorSetLayout,2> bindDescriptorSetLayouts;

    // created object
    VkPipeline graphicsPipeline{};
    VkPipelineLayout pipelineLayout{};

    // functions
    void init();
    void cleanup();
};



#endif //PIPELINE_H
