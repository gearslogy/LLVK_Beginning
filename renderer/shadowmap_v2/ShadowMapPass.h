//
// Created by lp on 2024/9/25.
//

#ifndef SHADOWMAP_PASS_H
#define SHADOWMAP_PASS_H
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"
#include "LLVK_UT_Pipeline.hpp"
LLVK_NAMESPACE_BEGIN
class VulkanRenderer;

struct ShadowMapGeometryContainer {
    struct RenderableObject {
        const GLTFLoader::Part *pGeometry;
        const VmaUBOKTX2Texture * pTexture; // rgba, a to clipping
        VkDescriptorSet set;// allocated set
    };

    struct RequiredObjects{
        const VulkanRenderer *pVulkanRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        const VmaUBOBuffer *pUBO;                          // ref:UBO binding=0 :depthMVP
        const VkDescriptorSetLayout *pSetLayout;           // used for create sets. we only one set, so one set layout
    };

    void setRequiredObjects( RequiredObjects &&rRequiredObjects);
    void addRenderableGeometry( RenderableObject obj);

    template<class Self>
    auto&& getRenderableObjects(this Self& self) {
        return std::forward<Self>(self).renderableObjects;
    }
private:
    // before create anything, we need fill this field
    RequiredObjects requiredObjects{};
    // all the geometry need to rendering depth
    std::vector<RenderableObject> renderableObjects{};
public:
    void buildSet();
};


class ShadowMapPass {
public:
    explicit ShadowMapPass(const VulkanRenderer* renderer, const VkDescriptorPool *descPool,
        const VkCommandBuffer *cmd);
    ~ShadowMapPass() = default;
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

    struct {
        VmaAttachment depthAttachment{};
        VkFramebuffer framebuffer{};
        VkRenderPass renderPass{};
        VkSampler depthSampler{};
    }shadowFramebuffer;

    // generated
    glm::mat4 depthMVP{};
    VmaUBOBuffer uboBuffer{};

    template<class Self>
    auto&& getGeometryContainer(this Self& self) {
        return std::forward<Self>(self).geoContainer;
    }
    void recordCommandBuffer();

private:
    void createOffscreenDepthAttachment();
    void createOffscreenRenderPass();
    void createOffscreenFramebuffer();
    void prepareDescriptorSets();
    void preparePipelines();

    VkPipeline offscreenPipeline{};
    VkDescriptorSetLayout offscreenDescriptorSetLayout{}; // only one set=0
    VkPipelineLayout offscreenPipelineLayout{};   //only 1 set: binding=0 UBO depthMVP, binding=1 colormap. use .a discard

    constexpr static void setRequiredObjects  (const auto *renderer, auto && ... ubo) {
        ((ubo.requiredObjects.device = renderer->getMainDevice().logicalDevice),...);
        ((ubo.requiredObjects.physicalDevice = renderer->getMainDevice().physicalDevice),...);
        ((ubo.requiredObjects.commandPool = renderer->getGraphicsCommandPool()),...);
        ((ubo.requiredObjects.queue = renderer->getGraphicsQueue()),...);
        ((ubo.requiredObjects.allocator = renderer->getVmaAllocator()),...);
    };


    const VulkanRenderer * pRenderer{VK_NULL_HANDLE};      // required object at ctor
    const VkDescriptorPool *pDescriptorPool{VK_NULL_HANDLE}; // required object at ctor
    const VkCommandBuffer *pCommandBuffer{VK_NULL_HANDLE};   // required object at ctor

    ShadowMapGeometryContainer geoContainer{};
    UT_GraphicsPipelinePSOs pipelinePSOs{};
};
LLVK_NAMESPACE_END


#endif //SHADOWMAP_PASS_H
