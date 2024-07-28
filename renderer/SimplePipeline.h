//
// Created by lp on 2024/7/13.
//

#ifndef SIMPLEPIPELINE_H
#define SIMPLEPIPELINE_H
#include "Pipeline.hpp"
LLVK_NAMESPACE_BEGIN
struct PipelineCache;
struct SimplePipeline {
    VkDevice bindDevice{};   // REF object
    VkRenderPass bindRenderPass{}; // REF object
    const PipelineCache *bindPipelineCache{}; // REF
    std::array<VkDescriptorSetLayout,2> bindDescriptorSetLayouts{};

    // created object
    VkPipeline graphicsPipeline{};
    VkPipelineLayout pipelineLayout{}; // sets/push constants

    // functions
    void init();
    void cleanup();
};
LLVK_NAMESPACE_END
#endif //SIMPLEPIPELINE_H
