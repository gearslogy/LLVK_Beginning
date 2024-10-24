//
// Created by lp on 2024/9/19.
//

#ifndef JSONSCENEPARSER_H
#define JSONSCENEPARSER_H
#include <LLVK_UT_Pipeline.hpp>
#include <libs/json.hpp>
#include "LLVK_GeomtryLoader.h"

#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class VulkanRenderer;

struct InstanceGeometryContainer {
    void buildSet();
    struct RenderableObject {
        const GLTFLoader::Part *pGeometry;
        // OUR CASE IS :
        //const VmaUBOKTX2Texture * pTexture;      // rgba, a to clipping
        //const VmaUBOKTX2Texture * pN;            //
        //const VmaUBOKTX2Texture * pOrdpTexture;  // RGBA:rough/metalness/ao/unkonw
        //const VmaUBOKTX2Texture * pDepthTexture; // depth
        std::vector<const IVmaUBOTexture *> pTextures; // use this to support multi textures
        VkDescriptorSet setUBO{VK_NULL_HANDLE};       // allocated set  set=0
        VkDescriptorSet setTexture{VK_NULL_HANDLE};   // set=1
        VkBuffer instanceBuffer{VK_NULL_HANDLE};      // binding point = 1
        void bindTextures(auto && ... textures) {(pTextures.emplace_back(textures), ... );}
    };
    struct RequiredObjects{
        const VulkanRenderer *pVulkanRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        const VmaUBOBuffer *pUBO;                          // ref:UBO binding=0  sceneUBO
        const VkDescriptorSetLayout *pSetLayoutUBO;        // set=0
        const VkDescriptorSetLayout *pSetLayoutTexture;    // set=1
    };
    void setRequiredObjects( RequiredObjects &&rRequiredObjects) { requiredObjects = rRequiredObjects;}
    void addRenderableGeometry( RenderableObject obj) { opaqueRenderableObjects.emplace_back(std::move(obj) );}
    template<class Self>
    auto&& getRenderableObjects(this Self& self) { return std::forward<Self>(self).opaqueRenderableObjects;}
private:
    RequiredObjects requiredObjects{};
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



struct InstancedObjectPass {
    InstancedObjectPass(const VulkanRenderer* renderer, const VkDescriptorPool *descPool);
    void cleanup();
    void prepare();
    VkBuffer loadInstanceData(std::string_view path);
private:
    void prepareUniformBuffers();
    void updateUniformBuffers(const glm::mat4 &depthMVP, const glm::vec4 &lightPos);
    void prepareDescriptorSets();
    void preparePipelines();
    void recordCommandBuffer();


    InstanceGeometryContainer geoContainer{};
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
    VmaUBOBuffer uboBuffer;

    VkDescriptorSetLayout uboDescSetLayout{};
    VkDescriptorSetLayout textureDescSetLayout{};
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline{};
};






LLVK_NAMESPACE_END


#endif //JSONSCENEPARSER_H
