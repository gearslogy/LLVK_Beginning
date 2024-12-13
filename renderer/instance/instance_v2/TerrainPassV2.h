//
// Created by lp on 2024/10/16.
//

#ifndef TERRAINPASS_H
#define TERRAINPASS_H


#include <LLVK_UT_Pipeline.hpp>

#include "LLVK_GeometryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class VulkanRenderer;

struct TerrainGeometryContainerV2 {

    struct RenderableObject {
        const GLTFLoader::Part *pGeometry;
        // OUR CASE IS :
        //const VmaUBOKTX2Texture * pTexture;      // rgba, a to clipping
        //const VmaUBOKTX2Texture * pOrdpTexture;  // ordp
        //const VmaUBOKTX2Texture * pN;            // B
        //const VmaUBOKTX2Texture * pDepthTexture; // depth
        std::vector<const IVmaUBOTexture *> pTextures; // use this to support multi textures
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setUBOs;       // set=0
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setTextures;   // set=1
        void bindTextures(auto && ... textures) {(pTextures.emplace_back(textures), ... );}
    };

    struct RequiredObjects{
        const VulkanRenderer *pVulkanRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        std::array<const VmaUBOBuffer *,MAX_FRAMES_IN_FLIGHT >pUBOs;          // MAX FLIGHT
        const VkDescriptorSetLayout *pSetLayoutUBO;        // set=0
        const VkDescriptorSetLayout *pSetLayoutTexture;    // set=1
    };

    void setRequiredObjects( RequiredObjects &&rRequiredObjects) { requiredObjects = rRequiredObjects;}

    void addRenderableGeometry( RenderableObject obj) {
        opaqueRenderableObjects.emplace_back(std::move(obj) );
    }

    template<class Self>
    auto&& getRenderableObjects(this Self& self) {
        return std::forward<Self>(self).opaqueRenderableObjects;
    }
private:
    RequiredObjects requiredObjects{};
    std::vector<RenderableObject> opaqueRenderableObjects{};
public:
    void buildSet();
};


class TerrainPassV2 {
public:
    TerrainPassV2(const VulkanRenderer* renderer, const VkDescriptorPool *descPool);
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

    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;

    TerrainGeometryContainerV2 geoContainer{};
    UT_GraphicsPipelinePSOs pipelinePSOs{};

    VkDescriptorSetLayout uboDescSetLayout{};
    VkDescriptorSetLayout textureDescSetLayout{};

    VkPipeline opaquePipeline{};      // used for grid render with depth map     forward rendering
    VkPipelineLayout pipelineLayout{};       // set=0 for UBO set=1 for texture

private:
    const VulkanRenderer * pRenderer{VK_NULL_HANDLE};      // required object at ctor
    const VkDescriptorPool *pDescriptorPool{VK_NULL_HANDLE}; // required object at ctor

};

LLVK_NAMESPACE_END

#endif //TERRAINPASS_H
