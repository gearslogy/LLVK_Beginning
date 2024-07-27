#pragma once
#include "VulkanRenderer.h"

#define OBJECT_INSTANCES 200

#include "GeoVertexDescriptions.h"
#include "BufferManager.h"


// Vertex layout for this example
// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
LLVK_NAMESPACE_BEGIN
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


namespace FnDescriptor {
    // (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT, arrayCount);
    inline VkDescriptorSetLayoutBinding setLayoutBinding(VkDescriptorType type, uint32_t binding,VkShaderStageFlagBits stageBit, uint32_t arrayCount= 1) {
        VkDescriptorSetLayoutBinding ret{};
        ret.binding = binding;
        ret.descriptorCount = arrayCount; // descriptorCount specifies the number of values in the array
        ret.descriptorType = type;
        ret.stageFlags = stageBit;
        ret.pImmutableSamplers = nullptr;
        return ret;
    }


    inline VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo(
        const VkDescriptorSetLayoutBinding* pBindings,
        uint32_t bindingCount){
        VkDescriptorSetLayoutCreateInfo ret {};
        ret.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ret.pBindings = pBindings;
        ret.bindingCount = bindingCount;
        return ret;
    }

    inline VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo(const Concept::is_range auto &bindings){
        VkDescriptorSetLayoutCreateInfo ret {};
        ret.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ret.pBindings = bindings.data();
        ret.bindingCount = bindings.size();
        return ret;
    }


    inline VkDescriptorPoolCreateInfo poolCreateInfo(
        const Concept::is_range auto&poolSizes,
        uint32_t maxSets)
    {
        VkDescriptorPoolCreateInfo ret{};
        ret.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ret.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        ret.pPoolSizes = poolSizes.data();
        ret.maxSets = maxSets;
        return ret;
    }
    inline VkDescriptorSetAllocateInfo setAllocateInfo(
                VkDescriptorPool descriptorPool,
                const VkDescriptorSetLayout* pSetLayouts,
                uint32_t descriptorSetCount)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
        descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
        return descriptorSetAllocateInfo;
    }

    inline VkDescriptorSetAllocateInfo setAllocateInfo(VkDescriptorPool descriptorPool,
        const Concept::is_range auto & layouts){
        return setAllocateInfo(descriptorPool, layouts.data(), layouts.size());
    }


    inline VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet dstSet,
        VkDescriptorType type,
        uint32_t binding,
        VkDescriptorBufferInfo* bufferInfo,
        uint32_t descriptorCount = 1){
        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pBufferInfo = bufferInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }

    inline VkWriteDescriptorSet writeDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType type,
        uint32_t binding,
        VkDescriptorImageInfo *imageInfo,
        uint32_t descriptorCount = 1){
        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pImageInfo = imageInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }

}

struct UBOBuffer {
    VkBuffer buffer{}; // ubo VS buffer
    VkDeviceMemory memory{};
    void * mapped{};
    VkDeviceSize memorySize{};
    VkDescriptorBufferInfo descBufferInfo{};

    VkDevice bindDevice{};
    VkPhysicalDevice bindPhysicalDevice{};
    void create(VkDeviceSize bufferSize, VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        assert(bindDevice!=nullptr);
        assert(bindPhysicalDevice!=nullptr);
        FnBuffer::createBuffer(bindPhysicalDevice, bindDevice, bufferSize,
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer,
                               memory);
        memorySize = bufferSize;
        descBufferInfo.buffer = buffer;
        descBufferInfo.offset = 0;
        //plantUniformBuffers.dynamicBuffer.descBufferInfo.range = dynamicAlignment;
        descBufferInfo.range = bufferSize; // in dynamic ubo, it't should be per block size.

    }
    void cleanup() {
        assert(bindDevice!=nullptr);
        assert(bindPhysicalDevice!=nullptr);
        vkDestroyBuffer(bindDevice, buffer, nullptr);
        vkFreeMemory(bindDevice, memory, nullptr);
    }
    void map() {
        if(vkMapMemory(bindDevice, memory, 0, memorySize, 0, &mapped) != VK_SUCCESS)
            throw std::runtime_error{"ERROR map memory"};
    }

};


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
        UBOBuffer viewBuffer{};
        UBOBuffer dynamicBuffer{};
    } plantUniformBuffers;

    struct {
        VkBuffer viewBuffer; // ubo VS buffer
        VkDeviceMemory viewMemory;
        void * viewMapped;
    } groundUniformBuffers;



    // Store random per-object rotations
    glm::vec3 rotations[OBJECT_INSTANCES];
    glm::vec3 rotationSpeeds[OBJECT_INSTANCES];


    size_t dynamicAlignment{ 0 };

    static constexpr int plantNumPlantsImages = 6;

    VkPipelineLayout plantPipelineLayout{ VK_NULL_HANDLE };
    VkDescriptorPool descriptorPool{};
    VkDescriptorSetLayout plantUBOSetLayout{ VK_NULL_HANDLE };     // set = 0
    VkDescriptorSetLayout plantTextureSetLayout{ VK_NULL_HANDLE }; // set = 1
    VkDescriptorSet plantDescriptorSets[2]{ VK_NULL_HANDLE };
    VkPipeline plantPipeline{ VK_NULL_HANDLE };
    std::array<ImageAndMemory, plantNumPlantsImages> plantsImageMems{};
    std::array<VkImageView, plantNumPlantsImages> plantsImageViews{};




    VkSampler sampler{};



    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
    void setupDescriptors();
    void preparePipelines();
    void prepareUniformBuffers();
    void updateUniformBuffers();
    void updateDynamicUniformBuffer();
    void bindResources();

    void recordCommandBuffer() override;

    void prepare() override {
        bindResources();
        loadTexture();
        loadModel();
        prepareUniformBuffers();
        setupDescriptors();
        preparePipelines();
    }
    void render() override {
        updateUniformBuffers();
        //updateDynamicUniformBuffer();
        recordCommandBuffer();
    }

    ObjLoader plantGeo{};
    ObjLoader groundGeo{};
    BufferManager geometryBufferManager{};

private:
    void loadPlantTextures();
    void loadGroundTextures();
    void createTextures(const Concept::is_range auto &files);
};

LLVK_NAMESPACE_END
