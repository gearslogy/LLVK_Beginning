#pragma once
#include "VulkanRenderer.h"

#define OBJECT_INSTANCES 125

#include "GeoVertexDescriptions.h"
#include "BufferManager.h"
#include "SimplePipeline.h"
// Vertex layout for this example
// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
inline void* alignedAlloc(size_t size, size_t alignment)
{
    void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
    data = _aligned_malloc(size, alignment);
#else
    int res = posix_memalign(&data, alignment, size);
    if (res != 0)
        data = nullptr;
#endif
    return data;
}

inline void alignedFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
    _aligned_free(data);
#else
    free(data);
#endif
}

struct DynamicsUBO : public VulkanRenderer{
    DynamicsUBO(): VulkanRenderer(){}

    // DATA:0
    struct {
        glm::mat4 projection;
        glm::mat4 view;
    } uboVS;

    // DATA:1, 必须GPU对齐
    struct UboDataDynamic {
        glm::mat4* model{ nullptr };
    } uboDataDynamic;

    // BUFFER
    struct {
        VkBuffer view; // uboVS buffer
        VkBuffer dynamic; // instance buffer
    } uniformBuffers;


    // Store random per-object rotations
    glm::vec3 rotations[OBJECT_INSTANCES];
    glm::vec3 rotationSpeeds[OBJECT_INSTANCES];


    VkPipeline pipeline{ VK_NULL_HANDLE };
    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };


    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
    void setupDescriptors();
    void preparePipelines();
    void updateUniformBuffer();
    void bindResources();

    void recordCommandBuffer() override;

    void prepare() override {
        bindResources();
        loadTexture();
        loadModel();
        setupDescriptors();
        preparePipelines();
    }
    void render() override {
        updateUniformBuffer();
        recordCommandBuffer();
    }

    ObjLoader simpleObjLoader{};
    BufferManager geometryBufferManager{};
    SimplePipeline simplePipeline{};
};


