//
// Created by lp on 2024/10/11.
//

#ifndef SCENEPASS_H
#define SCENEPASS_H

#include <LLVK_UT_Pipeline.hpp>

#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"
#include <unordered_map>
LLVK_NAMESPACE_BEGIN

class VulkanRenderer;

struct SceneGeometryContainer {
    enum ObjectPipelineTag{
        opaque = 1<<0,
        opacity = 1<<1,
    };

    struct RenderableObject {
        const GLTFLoader::Part *pGeometry;
        // OUR CASE IS :
        //const VmaUBOKTX2Texture * pTexture;      // rgba, a to clipping
        //const VmaUBOKTX2Texture * pOrdpTexture;  // ordp
        //const VmaUBOKTX2Texture * pDepthTexture; // depth
        std::vector<const IVmaUBOTexture *> pTextures; // use this to support multi textures
        VkDescriptorSet setUBO{VK_NULL_HANDLE};       // allocated set  set=0
        VkDescriptorSet setTexture{VK_NULL_HANDLE};   // set=1
    };

    struct RequiredObjects{
        const VulkanRenderer *pVulkanRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        const VmaUBOBuffer *pUBO;                          // ref:UBO binding=0  sceneUBO
        const VkDescriptorSetLayout *pSetLayoutUBO;        // set=0
        const VkDescriptorSetLayout *pSetLayoutTexture;    // set=1
    };

    void setRequiredObjects( RequiredObjects &&rRequiredObjects) { requiredObjects = rRequiredObjects;}

    void addRenderableGeometry( RenderableObject obj, ObjectPipelineTag tag) {
        if( tag == opacity) {
            opacityRenderableObjects.emplace_back(std::move(obj) );
        }
        else
            opaqueRenderableObjects.emplace_back(std::move(obj) );
    }

    template<class Self>
    auto&& getRenderableObjects(this Self& self, ObjectPipelineTag pipelineTag) {
        if(pipelineTag == opacity)
            return std::forward<Self>(self).opacityRenderableObjects;
        else return std::forward<Self>(self).opaqueRenderableObjects;
    }
private:
    // before create anything, we need fill this field
    RequiredObjects requiredObjects{};
    // all the geometry need to rendering depth
    std::vector<RenderableObject> opacityRenderableObjects{};
    std::vector<RenderableObject> opaqueRenderableObjects{};
    //std::unordered_map<ObjectPipelineTag>;
public:
    void buildSet();
};


class ScenePass {
public:
    ScenePass(const VulkanRenderer* renderer, const VkDescriptorPool *descPool);
    void prepare();
    void prepareUniformBuffers();
    void updateUniformBuffers(const glm::mat4 &depthMVP, const glm::vec4 &lightPos);
    void prepareDescriptorSets();
    void preparePipelines();
    void recordCommandBuffer();
    void cleanup();

    template<class Self>
    auto&& getGeometryContainer(this Self& self) {
        return std::forward<Self>(self).geoContainer;
    }

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

    VmaUBOBuffer uboBuffer;     // final rendering : opaque and opacity use same UBO


    SceneGeometryContainer geoContainer{};
    UT_GraphicsPipelinePSOs pipelinePSOs{};

    VkDescriptorSetLayout uboDescSetLayout{};
    VkDescriptorSetLayout textureDescSetLayout{};

    VkPipeline opacityPipeline{};     // used for foliage render with depth map. forward rendering
    VkPipeline opaquePipeline{};      // used for grid render with depth map     forward rendering
    VkPipelineLayout pipelineLayout{};       // set=0 for UBO set=1 for texture

private:
    const VulkanRenderer * pRenderer{VK_NULL_HANDLE};      // required object at ctor
    const VkDescriptorPool *pDescriptorPool{VK_NULL_HANDLE}; // required object at ctor


};
LLVK_NAMESPACE_END


#endif //SCENEPASS_H
