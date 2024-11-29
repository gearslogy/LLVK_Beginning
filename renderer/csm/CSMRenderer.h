//
// Created by liuya on 11/8/2024.
//

#ifndef CSMRENDERER_H
#define CSMRENDERER_H

#include "LLVK_GeomtryLoader.h"
#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"

LLVK_NAMESPACE_BEGIN
struct CSMDepthPass;
struct CSMScenePass;
class CSMRenderer : public VulkanRenderer {
    static constexpr int32_t CASCADE_COUNT{4};
    static constexpr int32_t shadow_map_size{2048};
public:
    friend struct CSMScenePass;
    friend struct CSMDepthPass;
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
    CSMRenderer();
    ~CSMRenderer();
    void prepare() override;
    void render() override;
    void cleanupObjects() override;

private:
    ResourceManager resourceManager{};
    void prepareUBOAndDesc();

    struct {
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
        glm::vec4 instancesPositions[4]; //29-buildings instance position
    }uboData{};

    using UBOFramedBuffers = std::array<VmaUBOBuffer, MAX_FRAMES_IN_FLIGHT>;
    using SetsFramed = std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>;

    UBOFramedBuffers uboGround{};
    UBOFramedBuffers ubo29{};
    UBOFramedBuffers ubo35{};
    UBOFramedBuffers ubo36{};
    UBOFramedBuffers ubo39{};
    SetsFramed setGround{};
    SetsFramed set29{};
    SetsFramed set35{};
    SetsFramed set36{};
    SetsFramed set39{};
    VkDescriptorSetLayout sceneDescLayout{};

    //depth sets

    VkDescriptorPool descPool{};
    std::unique_ptr<CSMScenePass> scenePass;

};
LLVK_NAMESPACE_END


#endif //CSMRENDERER_H
