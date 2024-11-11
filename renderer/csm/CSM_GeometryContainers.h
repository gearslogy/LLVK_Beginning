//
// Created by liuya on 11/11/2024.
//

#ifndef CSM_GEOMETRYCONTAINERS_H
#define CSM_GEOMETRYCONTAINERS_H


#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

// two set. binding N texture
// set=0 N ubos
// set=1 N textures
LLVK_NAMESPACE_BEGIN

class VulkanRenderer;
struct GenericGeometryContainers {
    struct RenderDelegate {
        const GLTFLoader::Part *pGeometry;
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
    void buildSet();
private:
    RequiredObjects requiredObjects{};
    std::vector<RenderDelegate> renderDelegates{};
};




LLVK_NAMESPACE_END

#endif //CSM_GEOMETRYCONTAINERS_H
