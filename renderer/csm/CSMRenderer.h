//
// Created by liuya on 11/8/2024.
//

#ifndef CSMRENDERER_H
#define CSMRENDERER_H

#include "LLVK_GeomtryLoader.h"
#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"
LLVK_NAMESPACE_BEGIN

struct CSMPass;
class CSMRenderer : public VulkanRenderer {
    static constexpr int32_t CASCADE_COUNT{4};
    static constexpr int32_t shadow_map_size{2048};
public:
    // RAII
    struct ResourceManager {
        CSMRenderer *pRenderer{};
        ~ResourceManager();

        void loading();
        struct {
            GLTFLoader ground;
            GLTFLoader geo_29;
            GLTFLoader geo_35;
            GLTFLoader geo_36;
            GLTFLoader geo_39;
            VmaSimpleGeometryBufferManager geometryBufferManager;
        }geos;

        struct {
            VmaUBOKTX2Texture d_tex_29;
            VmaUBOKTX2Texture d_tex_35;
            VmaUBOKTX2Texture d_tex_36;
            VmaUBOKTX2Texture d_tex_39;
            VmaUBOKTX2Texture d_ground;
        }textures;
        VkSampler colorSampler{};

    };
    void prepare() override;
    void render() override;
    void cleanupObjects() override;

private:
    ResourceManager resourceManager{};
    // depth relative objs
    void prepareOffscreenDepth();
    void prepareDepthRenderPass();
    void prepareFramebuffer();
    VkRenderPass depthRenderPass{};
    VkSampler depthSampler{};
    VmaAttachment depthAttachment{};
    VkFramebuffer depthFramebuffer{};


};
LLVK_NAMESPACE_END


#endif //CSMRENDERER_H
