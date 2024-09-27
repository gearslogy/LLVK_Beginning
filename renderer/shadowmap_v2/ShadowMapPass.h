//
// Created by lp on 2024/9/25.
//

#ifndef SHADOWMAP_PASS_H
#define SHADOWMAP_PASS_H
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class VulkanRenderer;

struct ShadowMapGeometry {
    struct RequiredObjects {
        const GLTFLoader *pGeometry;
        const VmaUBOBuffer *pDepthMVP_UBO;
        const VkDescriptorPool *pPool;
        std::vector<VmaUBOKTX2Texture *> pTextures; // one geometry container multi textures
    };
    RequiredObjects requiredObjects;

    VkDescriptorSet set;
    VkDescriptorSetLayout setLayout; // one set
    VkPipelineLayout pipelineLayout;

    void render();
    void cleanup();

};


struct ShadowMapPassRequiredObjects {
    const VmaUBOKTX2Texture *map; // color map. RGBA, a to drop texture
    std::vector<GLTFLoader *> geometries ;
};

class ShadowMapPass {
public:
    explicit ShadowMapPass(VulkanRenderer* pRenderer);
    constexpr static uint32_t depth_width = 2048;
    constexpr static uint32_t depth_height = depth_width;
    void prepare();
    void cleanup();


    void prepareUniformBuffers();
    void updateUniformBuffers();

    // param to setting
    glm::vec3 lightPos{};
    float near{};
    float far{};

    // generated
    glm::mat4 depthMVP{};
    VmaUBOBuffer uboBuffer{};

    // required object
    ShadowMapPassRequiredObjects requiredObjects;

private:
    void createOffscreenDepthAttachment();
    void createOffscreenRenderPass();
    void createOffscreenFramebuffer();

    void prepareDescriptorSets();
    void preparePipelines();
private:
    VkPipeline offscreenPipeline{};
    VkDescriptorSetLayout offscreenDescriptorSetLayout{}; // only one set=0
    VkPipelineLayout offscreenPipelineLayout{};   //only 1 set: binding=0 UBO depthMVP, binding=1 colormap. use .a discard

    constexpr static void setRequiredObjects  (const auto *renderer, auto && ... ubo) {
        ((ubo.requiredObjects.device = renderer->mainDevice.logicalDevice),...);
        ((ubo.requiredObjects.physicalDevice = renderer->mainDevice.physicalDevice),...);
        ((ubo.requiredObjects.commandPool = renderer->graphicsCommandPool),...);
        ((ubo.requiredObjects.queue = renderer->mainDevice.graphicsQueue),...);
        ((ubo.requiredObjects.allocator = renderer->vmaAllocator),...);
    };

    struct {
        VmaAttachment depthAttachment{};
        VkFramebuffer framebuffer{};
        VkRenderPass renderPass{};
        VkSampler depthSampler{};
    }shadowFramebuffer;

    VulkanRenderer * renderer{};
};
LLVK_NAMESPACE_END


#endif //SHADOWMAP_PASS_H
