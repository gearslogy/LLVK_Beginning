//
// Created by liuya on 11/11/2024.
//

#ifndef CSM_GEOMETRYCONTAINERS_H
#define CSM_GEOMETRYCONTAINERS_H


#include "../../LLVK_GeomtryLoader.h"
#include "../../LLVK_VmaBuffer.h"


LLVK_NAMESPACE_BEGIN

class VulkanRenderer;
struct SetID0{};
struct SetID1{};
struct WriteSetDstBinding_BD_0{};
struct WriteSetDstBinding_BD_0_N{};
struct WriteSetDstBinding_BD_1_N{};


/*
template<SetNumType SetNumType>
struct RenderDelegate;

template<>
struct RenderDelegate<SetNumType::SET0_UBO_BINDING_0_N> {
    const GLTFLoader::Part *pGeometry;
    std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setUBOs;
};

template<>
struct RenderDelegate<SetNumType::SET0_UBO_BINDING_0_TEXTURE_BINDING_1_N> {
    const GLTFLoader::Part *pGeometry;
    std::vector<const IVmaUBOTexture *> pTextures; // use this to support multi textures
    std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT* 2 > sets; // ubo tex | ubo tex |
    void bindTextures(auto && ... textures) {(pTextures.emplace_back(textures), ... );}
};

// Required object interface
template<SetNumType SetNumType>
struct RequiredObjects;

template<>
struct RequiredObjects<SetNumType::SET0_UBO_BINDING_0_N> {
    const VulkanRenderer *pVulkanRenderer;
    const VkDescriptorPool *pPool;                     // ref:pool allocate sets
    std::array<const VmaUBOBuffer *,MAX_FRAMES_IN_FLIGHT >pUBOs;          // MAX FLIGHT
    const VkDescriptorSetLayout *pSetLayoutUBO;        // set=0
};

template<>
struct RequiredObjects<SetNumType::SET0_UBO_BINDING_0_TEXTURE_BINDING_1_N> {
    const VulkanRenderer *pVulkanRenderer;
    const VkDescriptorPool *pPool;                     // ref:pool allocate sets
    std::array<const VmaUBOBuffer *,MAX_FRAMES_IN_FLIGHT >pUBOs;          // MAX FLIGHT
    const VkDescriptorSetLayout *pSetLayout;        // set=0
};
*/



struct RenderContainerTwoSet {
    struct RenderDelegate {
        const GLTFLoader::Part *pGeometry;
        std::vector<const IVmaUBOTexture *> pTextures; // use this to support multi textures
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setUBOs;       // set=0 for ubo
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setTextures;   // set=1 for tex
        void bindTextures(auto && ... textures) {(pTextures.emplace_back(textures), ... );}
    };

    struct RequiredObjects{
        const VulkanRenderer *pVulkanRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        std::array<const VmaUBOBuffer *,MAX_FRAMES_IN_FLIGHT >pUBOs;          // MAX FLIGHT
        const VkDescriptorSetLayout *pSetLayoutUBO;        // set=0 for ubo
        const VkDescriptorSetLayout *pSetLayoutTexture;    // set=1 for tex
    };
    void setRequiredObjects( RequiredObjects &&rRequiredObjects) { requiredObjects = rRequiredObjects;}
    void buildSet();
private:
    RequiredObjects requiredObjects{};
    std::vector<RenderDelegate> renderDelegates{};
};

struct RenderContainerOneSet {
    struct RenderDelegate {
        const GLTFLoader::Part *pGeometry;
        std::vector<const IVmaUBOTexture *> pTextures; // use this to support multi textures
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> sets;       // one set
        void bindTextures(auto && ... textures) {(pTextures.emplace_back(textures), ... );}
    };
    struct RequiredObjects{
        const VulkanRenderer *pVulkanRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        std::array<const VmaUBOBuffer *,MAX_FRAMES_IN_FLIGHT >pUBOs;          // MAX FLIGHT
        const VkDescriptorSetLayout *pSetLayout;        // set=0
    };
    void setRequiredObjects( RequiredObjects &&rRequiredObjects) { requiredObjects = rRequiredObjects;}
    void cmdBindDescriptorSets();
    void buildSet();
private:
    RequiredObjects requiredObjects{};
    std::vector<RenderDelegate> renderDelegates{};
};





LLVK_NAMESPACE_END

#endif //CSM_GEOMETRYCONTAINERS_H
