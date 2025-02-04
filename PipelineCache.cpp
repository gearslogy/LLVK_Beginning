//
// Created by lp on 2024/7/2.
//

#include "PipelineCache.h"
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>
#include <libs/magic_enum.hpp>
LLVK_NAMESPACE_BEGIN
    constexpr const char* cacheFolder = "content/engine";
constexpr const char* cacheName = "pipeline_cache.bin";
namespace fs = std::filesystem;
void PipelineCache::init() {
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
}

void PipelineCache::loadCache() {
    auto path = fs::path(cacheFolder) / fs::path(cacheName);
    std::ifstream cacheFile(path, std::ios::binary);
    if(cacheFile) {
        std::vector<uint8_t> cacheData((std::istreambuf_iterator<char>(cacheFile)), std::istreambuf_iterator<char>());
        cacheFile.close();
        // header version checking
        auto *cacheHeader = reinterpret_cast<VkPipelineCacheHeaderVersionOne *>(cacheData.data());
        std::cout << std::format("[[PipelineCache::loadCache]]: headerVersion:{} vendorID:{}\n" ,magic_enum::enum_name(cacheHeader->headerVersion),cacheHeader->vendorID );

        /*
        VkPhysicalDeviceProperties deviceProperties;
        cacheHeader->deviceID ==deviceProperties.deviceID ;// checking
        cacheHeader->vendorID == deviceProperties.vendorID ;//
        memcmp(cacheHeader->pipelineCacheUUID,deviceProperties.pipelineCacheUUID,VK_UUID_SIZE) == 0;*/


        pipelineCacheCreateInfo.initialDataSize = cacheData.size();
        pipelineCacheCreateInfo.pInitialData = cacheData.data();
    }
    if (vkCreatePipelineCache(bindDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline cache!");
    }

}

void PipelineCache::writeCache() {
    auto path = fs::path(cacheFolder) ;
    fs::create_directories(path);
    auto file = fs::path(cacheFolder) / fs::path(cacheName);

    size_t cacheSize;
    vkGetPipelineCacheData(bindDevice, pipelineCache, &cacheSize, nullptr);
    std::vector<uint8_t> cacheData(cacheSize);
    vkGetPipelineCacheData(bindDevice, pipelineCache, &cacheSize, cacheData.data());

    std::ofstream cacheFileOut(file, std::ios::binary);
    if (cacheFileOut) {
        cacheFileOut.write(reinterpret_cast<const char*>(cacheData.data()), cacheData.size());
        cacheFileOut.close();
    } else {
        throw std::runtime_error("failed to open file for writing pipeline cache data!");
    }
}
void PipelineCache::cleanup() {
    vkDestroyPipelineCache(bindDevice,pipelineCache, nullptr);
}
LLVK_NAMESPACE_END
