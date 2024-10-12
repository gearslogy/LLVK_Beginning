//
// Created by lp on 2024/10/11.
//

#ifndef SCENEPASS_H
#define SCENEPASS_H

#include <LLVK_UT_Pipeline.hpp>

#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"
LLVK_NAMESPACE_BEGIN

class VulkanRenderer;

struct SceneGeometryContainer {
    struct RenderableObject {
        const GLTFLoader::Part *pGeometry;
        // OUR CASE IS :
        //const VmaUBOKTX2Texture * pTexture;      // rgba, a to clipping
        //const VmaUBOKTX2Texture * pOrdpTexture;  // ordp
        //const VmaUBOKTX2Texture * pDepthTexture; // depth
        std::vector<const VmaUBOKTX2Texture *> pTextures; // use this to support multi textures
        VkDescriptorSet setUBO;       // allocated set  set=0
        VkDescriptorSet setTexture;   // set=1
    };

    struct RequiredObjects{
        const VulkanRenderer *pVulkanRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        const VmaUBOBuffer *pUBO;                          // ref:UBO binding=0  sceneUBO
        const VkDescriptorSetLayout *pSetLayoutUBO;        // set=0
        const VkDescriptorSetLayout *pSetLayoutTexture;    // set=1
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


class ScenePass {
public:
    ScenePass(const VulkanRenderer* renderer, const VkDescriptorPool *descPool,
       const VkCommandBuffer *cmd);
    void prepare();
    void prepareUniformBuffers();
    void updateUniformBuffers();
    void prepareDescriptorSets();
    void preparePipelines();
    void recordCommandBuffer();
    void cleanup();
private:
    struct UniformDataScene {
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model;
        glm::mat4 depthBiasMVP;
        glm::vec4 lightPos;
        // Used for depth map visualization
        float zNear{0.1};
        float zFar{1000.0};
    } uniformDataScene;


    SceneGeometryContainer geoContainer{};
    UT_GraphicsPipelinePSOs pipelinePSOs{};

    VkDescriptorSetLayout uboDescSetLayout{};
    VkDescriptorSetLayout textureDescSetLayout{};

private:
    const VulkanRenderer * pRenderer{VK_NULL_HANDLE};      // required object at ctor
    const VkDescriptorPool *pDescriptorPool{VK_NULL_HANDLE}; // required object at ctor
    const VkCommandBuffer *pCommandBuffer{VK_NULL_HANDLE};   // required object at ctor

private:


};
LLVK_NAMESPACE_END


#endif //SCENEPASS_H
