//
// Created by lp on 2024/7/2.
//

#ifndef PIPELINECACHE_H
#define PIPELINECACHE_H

#include <vulkan/vulkan.h>

struct PipelineCache {
    VkDevice bindDevice{};

    void init();
    void cleanup();

    VkPipelineCache pipelineCache{};
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
    void writeCache();
    void loadCache();

};



#endif //PIPELINECACHE_H
