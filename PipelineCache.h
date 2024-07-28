//
// Created by lp on 2024/7/2.
//

#ifndef PIPELINECACHE_H
#define PIPELINECACHE_H

#include <vulkan/vulkan.h>
#include "LLVK_SYS.hpp"
LLVK_NAMESPACE_BEGIN
struct PipelineCache {
    VkDevice bindDevice{};

    void init();
    void cleanup();

    VkPipelineCache pipelineCache{};
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
    void writeCache();
    void loadCache();

};
LLVK_NAMESPACE_END


#endif //PIPELINECACHE_H
