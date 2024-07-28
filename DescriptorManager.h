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
#include "LLVK_Image.h"
#include "Utils.h"
LLVK_NAMESPACE_BEGIN
struct LayoutBindings {
    static std::array<VkDescriptorSetLayoutBinding,2> getUBODescriptorSetLayoutBindings(VkDevice device);
    static std::array<VkDescriptorSetLayoutBinding,1> getTextureDescriptorSetLayoutBindings(VkDevice device);
    static VkDescriptorSetLayout createUBODescriptorSetLayout(VkDevice device);
    static VkDescriptorSetLayout createTextureDescriptorSetLayout(VkDevice device);
};


struct alignas(16) UBO1 {
    alignas(16) glm::vec2 screenSize;
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
    VkPhysicalDevice bindPhysicalDevice;
    VkCommandPool bindCommandPool;
    VkQueue bindQueue;
    const VkExtent2D *bindSwapChainExtent;


    void createDescriptorSetLayout();
    void createTexture(const char *tex);
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    // 我们一帧将构建2个set，一个UBO，一个是texture
    VkDescriptorSetLayout ubo_descriptorSetLayout;      // only for control cbuffer
    VkDescriptorSetLayout texture_descriptorSetLayout;  // only for control texture

    UniformBuffers_FrameFlighted simpleUniformBuffer;
    VkDescriptorPool descriptorPool;

    /*假设 MAX_FRAME_FLIGHTED = 2
                              帧0                          帧1
    构建4个set.实际会开辟4个set.[ubo_layout,texture_layout,   ubo_layout,texture_layout]
    ubo的set = 0
    texture 的set = 1
    */
    std::vector<VkDescriptorSet> descriptorSets; // 每帧2个set, 所以4个
    static constexpr int numCreatedSets = 2 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) ; // 每帧2个set, 所以4个


    // 生成texture
    ImageAndMemory imageAndMemory;
    VkImageView imageView;
    VkSampler imageSampler;

    void cleanup();
    static constexpr uint32_t num_ubos = 2;
};

LLVK_NAMESPACE_END

#endif //DESCRIPTORMANAGER_H
