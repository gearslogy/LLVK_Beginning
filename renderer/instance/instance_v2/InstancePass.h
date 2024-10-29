//
// Created by lp on 2024/9/19.
//

#ifndef JSONSCENEPARSER_H
#define JSONSCENEPARSER_H
#include <LLVK_UT_Pipeline.hpp>
#include <libs/json.hpp>
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class VulkanRenderer;


struct InstanceDesc {
    VkBuffer buffer{VK_NULL_HANDLE};
    size_t drawCount{0}; // instance count
};

struct InstanceGeometryContainer {
    enum ObjectPipelineTag{
        opaque,
        opacity
    };
    void buildSet();
    struct RenderableObject {
        const GLTFLoader::Part *pGeometry;
        // OUR CASE IS :
        //const VmaUBOKTX2Texture * pTexture;      // rgba, a to clipping
        //const VmaUBOKTX2Texture * pN;            //
        //const VmaUBOKTX2Texture * pOrdpTexture;  // RGBA:rough/metalness/ao/unkonw
        //const VmaUBOKTX2Texture * pDepthTexture; // depth
        std::vector<const IVmaUBOTexture *> pTextures; // use this to support multi textures
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setUBOs;       // set=0
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setTextures;   // set=1
        InstanceDesc instDesc{};
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
    void addRenderableGeometry( RenderableObject obj, ObjectPipelineTag tag) {
        if( tag == opacity) opacityRenderableObjects.emplace_back(std::move(obj) );
        else opaqueRenderableObjects.emplace_back(std::move(obj) );
    }
    template<class Self>
    auto&& getRenderableObjects(this Self& self, ObjectPipelineTag pipelineTag) {
        if(pipelineTag == opacity)
            return std::forward<Self>(self).opacityRenderableObjects;
        else return std::forward<Self>(self).opaqueRenderableObjects;
    }
private:
    RequiredObjects requiredObjects{};
    std::vector<RenderableObject> opacityRenderableObjects{};
    std::vector<RenderableObject> opaqueRenderableObjects{};

};

struct InstanceData {
    glm::vec3 P;
    glm::vec4 orient;
    float pscale;
};



struct JsonPointsParser {
    explicit JsonPointsParser(const std::string &path);
    nlohmann::json jsHandle;
    std::vector< InstanceData> instanceData;
};



struct InstancePass {
    InstancePass(const VulkanRenderer* renderer, const VkDescriptorPool *descPool);
    void cleanup();
    void prepare();
    InstanceDesc loadInstanceData(std::string_view path);
    InstanceGeometryContainer geoContainer{};
    void updateUniformBuffers(const glm::mat4 &depthMVP, const glm::vec4 &lightPos);
    void recordCommandBuffer();
private:
    void prepareUniformBuffers();
    void prepareDescriptorSets();
    void preparePipelines();


    VmaSimpleGeometryBufferManager instanceBufferManager{};
    UT_GraphicsPipelinePSOs pipelinePSOs;

    const VulkanRenderer * pRenderer{VK_NULL_HANDLE};      // required object at ctor
    const VkDescriptorPool *pDescriptorPool{VK_NULL_HANDLE}; // required object at ctor

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

    VkDescriptorSetLayout uboDescSetLayout{};
    VkDescriptorSetLayout textureDescSetLayout{};
    VkPipelineLayout pipelineLayout;
    VkPipeline opacityPipeline{}; // for rendering leaves
    VkPipeline opaquePipeline{};  // for rendering tree-root
};






LLVK_NAMESPACE_END


#endif //JSONSCENEPARSER_H
