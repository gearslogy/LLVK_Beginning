//
// Created by liuya on 11/11/2024.
//

#ifndef CSM_GEOMETRYCONTAINERS_H
#define CSM_GEOMETRYCONTAINERS_H


#include "../../LLVK_GeomtryLoader.h"
#include "../../LLVK_VmaBuffer.h"


LLVK_NAMESPACE_BEGIN

class VulkanRenderer;

namespace UT_GeometryContainer{
    template<typename T>
    concept is_gltf_part = std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, GLTFLoader::Part>;

    inline void renderPart(VkCommandBuffer cmdBuf, const GLTFLoader::Part * part, uint32_t instanceCount = 1) {
        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &part->verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,part->indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuf, part->indices.size(), instanceCount, 0, 0, 0);
    }

}

struct GeometryContainers {
    using Array_pTextures_t = std::vector<const VmaUBOTexture *>;
    using Array_pRenderDelegates_t = std::vector<const GLTFLoader::Part *>;
};


// two set, one ubo, multi textures
struct RenderContainerTwoSet {
    struct RenderDelegate {
        const GLTFLoader::Part *pGeometry;
        std::vector<const IVmaUBOTexture *> pTextures; // use this to support multi textures
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setUBOs;       // set=0 for ubo
        std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setTextures;   // set=1 for tex
        void bindTextures(auto && ... textures) {(pTextures.emplace_back(textures), ... );}
    };

    struct RequiredObjects{
        const VulkanRenderer *pRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        std::array<const VmaUBOBuffer *,MAX_FRAMES_IN_FLIGHT >pUBOs;          // MAX FLIGHT
        const VkDescriptorSetLayout *pSetLayoutUBO;        // set=0 for ubo
        const VkDescriptorSetLayout *pSetLayoutTexture;    // set=1 for tex
    };
    void setRequiredObjects( RequiredObjects &&rRequiredObjects) { requiredObjects = rRequiredObjects;}
    void buildSet();
    void draw(const VkCommandBuffer &cmdBuf, const VkPipelineLayout &pipelineLayout);

    RequiredObjects requiredObjects{};
    std::vector<RenderDelegate> renderDelegates{};
};


// one set, one ubo, multi textures
struct RenderContainerOneSet {
    struct RenderDelegate {
        friend struct RenderContainerOneSet;
        void bindGeometry(const GLTFLoader::Part *pGeo){ pGeometry = pGeo;}
        void bindTextures(auto && ... textures) {(pTextures.emplace_back(textures), ... );}
        template<class Self>
        auto&& getSet(this Self& self,uint32_t flightFrame) {return std::forward<Self>(self).descSets[flightFrame];}
    private:
        const GLTFLoader::Part *pGeometry{};
        std::vector<const IVmaUBOTexture *> pTextures{}; // use this to support multi textures
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descSets{};       // created sets
    };
    struct RequiredObjects{
        const VulkanRenderer *pRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
        std::array<const VmaUBOBuffer *,MAX_FRAMES_IN_FLIGHT >pUBOs;          // MAX FLIGHT
        const VkDescriptorSetLayout *pSetLayout;        // set=0
    };
    void setRequiredObjects( RequiredObjects &&rRequiredObjects) { requiredObjects = rRequiredObjects;}
    void draw(const VkCommandBuffer &cmdBuf,const VkPipelineLayout &pipelineLayout);
    void buildSet();

    RequiredObjects requiredObjects{};
    std::vector<RenderDelegate> renderDelegates{};
};


// one set, one ubo, multi textures
struct RenderContainerOneSharedSet {
    struct RequiredObjects{
        const VulkanRenderer *pRenderer;
        const VkDescriptorPool *pPool;                     // ref:pool allocate sets
    };

    void setRequiredObjects( RequiredObjects &&rRequiredObjects) { requiredObjects = rRequiredObjects;}
    void draw(const VkCommandBuffer &cmdBuf,const VkPipelineLayout &pipelineLayout);

    // call on descriptor set building
    void buildSets(const VkDescriptorSetLayout *pSetLayout);

    // resource bind
    void bindGeometries(Concept::is_range auto rangeGeos) {
        using vec_t = std::remove_cvref_t<decltype(rangeGeos)>;
        if constexpr (std::is_same_v<vec_t, GeometryContainers::Array_pRenderDelegates_t>)
            renderGeos = std::move(rangeGeos);
        else
            renderGeos.assign(rangeGeos.begin(), rangeGeos.end());
    }
    void bindGeometries(const UT_GeometryContainer::is_gltf_part auto *... geos) { (renderGeos.emplace_back(geos) , ... );}
    void bindTextures(auto && ... textures) {(pTextures.emplace_back(textures), ... );}

private:
    template<class Self>
    auto&& getSet(this Self& self,uint32_t flightFrame) {  return std::forward<Self>(self).descSets[flightFrame]; }

    GeometryContainers::Array_pRenderDelegates_t renderGeos{};
    GeometryContainers::Array_pTextures_t pTextures{}; // use this to support multi textures
    std::array<const VmaUBOBuffer *,MAX_FRAMES_IN_FLIGHT > pUBOs{};          // MAX FLIGHT
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descSets{};    // created sets
    RequiredObjects requiredObjects{};
};


LLVK_NAMESPACE_END

#endif //CSM_GEOMETRYCONTAINERS_H
