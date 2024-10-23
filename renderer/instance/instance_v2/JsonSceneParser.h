//
// Created by lp on 2024/9/19.
//

#ifndef JSONSCENEPARSER_H
#define JSONSCENEPARSER_H
#include <libs/json.hpp>
#include "LLVK_GeomtryLoader.h"

#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class VulkanRenderer;

struct InstanceGeometryContainer {
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
public:
    void buildSet();
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



class InstancedObjectPass {
    InstancedObjectPass(const VulkanRenderer* renderer, const VkDescriptorPool *descPool);
    void cleanup();
private:
    void prepareInstanceData();
    VmaSimpleGeometryBufferManager instanceBufferManager{};

private:
    const VulkanRenderer * pRenderer{VK_NULL_HANDLE};      // required object at ctor
    const VkDescriptorPool *pDescriptorPool{VK_NULL_HANDLE}; // required object at ctor

};






LLVK_NAMESPACE_END


#endif //JSONSCENEPARSER_H
