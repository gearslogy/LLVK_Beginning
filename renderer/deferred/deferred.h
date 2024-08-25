//
// Created by liuya on 8/16/2024.
//

#ifndef DEFER_H
#define DEFER_H

#include "LLVK_GeomtryLoader.h"
#include "VulkanRenderer.h"
#include "LLVK_VmaBuffer.h"
LLVK_NAMESPACE_BEGIN



struct defer :  VulkanRenderer{
    defer();
    struct {
        // camera
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
        // instance
        glm::vec4 instancePos[3];
        glm::vec4 instanceRot[3];
        glm::vec4 instanceScale[3];
    }mrtData{};// vert ubo

    struct Light {
        glm::vec4 position;
        glm::vec3 color;
        float radius;
    };
    struct {
        Light lights[5];
        glm::vec4 viewPos;
    }compositionData{}; // frag ubo

    struct {
        VmaUBOBuffer mrt;
        VmaUBOBuffer composition;
    }uniformBuffers{};

    struct {
        VkPipeline mrt;
        VkPipeline composition;
        VkPipelineLayout mrtLayout;
        VkPipelineLayout compositionLayout;
    }pipelines{};

    // Geos
    GLTFLoader ground_gltf{};
    GLTFLoader skull_gltf{};
    VmaSimpleGeometryBufferManager simpleGeoBufferManager{};

    struct {
        //layout(set=1, binding = 0) uniform sampler2D AlbedoTexSampler;
        //layout(set=1, binding = 1) uniform sampler2D NormalTexSampler;
        //layout(set=1, binding = 2) uniform sampler2D RoughnessTexSampler;
        //layout(set=1, binding = 3) uniform sampler2D DisplaceTexSampler;
        constexpr auto tex_count = 4;
        std::array<VmaUBOTexture,tex_count> ground_textures{};
        std::array<VmaUBOTexture,tex_count> skull_textures{};
    }UBOTextures;


    VkDescriptorPool descPool;
    struct {
        VkDescriptorSetLayout setLayout0; // set=0 for ubo: vertex shader bit
        VkDescriptorSetLayout setLayout1; // set=1 for texture
        VkDescriptorSet ground[2];
        VkDescriptorSet skull[2];
    } geoDescriptorSets{};

    struct {
        //layout(set=1, binding = 0) uniform sampler2D position;
        //layout(set=1, binding = 1) uniform sampler2D normal;
        //layout(set=1, binding = 2) uniform sampler2D albedo;
        //layout(set=1, binding = 3) uniform sampler2D roughness;
        //layout(set=1, binding = 4) uniform sampler2D displace;
        constexpr auto tex_count = 5;
        VkDescriptorSetLayout setLayout0; // set=0 for ubo: fragment shader bit
        VkDescriptorSetLayout setLayout1; // set=1 for texture
        VkDescriptorSet composition[2]; // set=0 UBO, set=1 texture
    }compositionDescriptorSets;


    struct FrameBuffer {
        IVmaUBOTexture position, normal, albedo, roughness, displace;
        IVmaUBOTexture depth;
        VkRenderPass renderPass{};
        VkFramebuffer frameBuffer{};
    } mrtFrameBuf{};
    VkSampler colorSampler{};

    void prepareMrtRenderPass(); // 1
    void prepareMrtFramebuffer(); // 2
    void cleanupMrtFramebuffer();
    void prepareUniformBuffers(); // 4
    void prepareDescriptorSets(); // 3
    void updateUniformBuffers();
    void preparePipelines();      // 5
    void loadModels();            // 6
    void loadTextures();          // 7
    void recordMrtCommandBuffer();
    void recordCompositionCommandBuffer();

    void cleanupObjects() override;
    void swapChainResize() override;


    void prepare() override {
        prepareMrtRenderPass();  // 1
        prepareMrtFramebuffer(); // 2
        prepareUniformBuffers(); // 3
        prepareDescriptorSets(); // 4
        preparePipelines();      // 5
        loadModels();            // 6
        loadTextures();          // 7
    }
    void render() override;


private:
    inline void setRequiredObjects  (auto && ... ubo) {
        ((ubo.requiredObjects.device = this->mainDevice.logicalDevice),...);
        ((ubo.requiredObjects.physicalDevice = this->mainDevice.physicalDevice),...);
        ((ubo.requiredObjects.commandPool = this->graphicsCommandPool),...);
        ((ubo.requiredObjects.queue = this->mainDevice.graphicsQueue),...);
        ((ubo.requiredObjects.allocator = this->vmaAllocator),...);
    };

    void createAttachment(const VkFormat &format,
        const VkImageUsageFlagBits &usage,
        IVmaUBOTexture & attachment);



};



LLVK_NAMESPACE_END

#endif //DEFER_H
