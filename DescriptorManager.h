//
// Created by star on 5/12/2024.
//

#ifndef DESCRIPTORMANAGER_H
#define DESCRIPTORMANAGER_H

#include <vulkan/vulkan.h>
#include <vector>
#include "BufferManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::rotate
struct LayoutBindings {
    static std::array<VkDescriptorSetLayoutBinding,2> getDescriptorSetLayoutBindings(VkDevice device);
    static VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device);
};


struct alignas(16) UBO1 {
    glm::vec3 dynamicsColor; // 4byte * 3
    alignas(16) glm::mat4 model{1.0f}; // 4byte * 4
    alignas(16) glm::mat4 view{1.0f};  // 4byte * 4
    alignas(16) glm::mat4 proj{1.0f};  // 4byte * 4
};

struct UBO2 {
    float baseAmp;     // 4
    float specularAmp; // 4
    alignas(16) glm::vec4 base;
    alignas(16) glm::vec4 specular;
    alignas(16) glm::vec4 normal;
    alignas(16) glm::vec3 aa;
    alignas(16) glm::vec4 bb;
};

struct UniformBufferMemoryAndMappedMemory {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *mapped;
    void cleanup(VkDevice device) {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device,memory, nullptr);
    }
};

struct UniformBuffers_FrameFlighted {
    VkDevice bindDevice;
    VkPhysicalDevice bindPhysicalDevice;
    const VkExtent2D *bindSwapChainExtent;
    void createUniformBuffers();
    void cleanup();
    void updateUniform(uint32_t currentFrame);
    std::vector<UniformBufferMemoryAndMappedMemory> uboObject1;
    std::vector<UniformBufferMemoryAndMappedMemory> uboObject2;
};


/* CODE ORDER
createInstance();
setupDebugMessenger();
createSurface();
pickPhysicalDevice();
createLogicalDevice();
createSwapChain();
createImageViews();
createRenderPass();
createDescriptorSetLayout();
createGraphicsPipeline();
createFramebuffers();
createCommandPool();
createVertexBuffer();
createIndexBuffer();
createUniformBuffers();
createDescriptorPool();
createDescriptorSets();
createCommandBuffers();
createSyncObjects();

*/
struct DescriptorManager {
    VkDevice bindDevice;
    VkPhysicalDevice bindPhyiscalDevice;
    const VkExtent2D *bindSwapChainExtent;
    void createDescriptorSetLayout();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    VkDescriptorSetLayout descriptorSetLayout;
    UniformBuffers_FrameFlighted simpleUniformBuffer;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets; // 每帧一个

    void cleanup();
    static constexpr uint32_t num_ubos = 2;
};



#endif //DESCRIPTORMANAGER_H
