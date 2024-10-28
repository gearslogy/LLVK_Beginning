//
// Created by liuya on 8/16/2024.
//

#pragma once

#include "VulkanRenderer.h"
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN

struct InstancePass;
struct TerrainPassV2;
struct Resources {
    void loading();
    void cleanup();

    VulkanRenderer *pRenderer{nullptr};
    InstancePass *pInstancedObjectPass{nullptr};
    TerrainPassV2 *pTerrainPass{nullptr};
    struct  {
        GLTFLoader terrain{};// only use part0
        GLTFLoader tree{};
        VmaSimpleGeometryBufferManager geoBufferManager{};
    }geos;

    struct {
        // simulate terrain
        VmaUBOKTX2Texture albedoArray{};
        VmaUBOKTX2Texture normalArray{};
        VmaUBOKTX2Texture ordpArray{};
        VmaUBOKTX2Texture mask{};
    }terrainTextures;

    struct {
        std::array<VmaUBOKTX2Texture,3> leaves; // albedo  &N  &mix
        std::array<VmaUBOKTX2Texture,3> branch; // albedo  &N  &mix
        std::array<VmaUBOKTX2Texture,3> root;   // albedo  &N  &mix
    }treeTextures;
    VkSampler colorSampler{};
private:
    void loadTerrain();
    void loadTree();
};




struct InstanceRendererV2 final : VulkanRenderer {
    InstanceRendererV2();
    ~InstanceRendererV2() override;
    void cleanupObjects() override;
    void loadResources();
    void createDescriptorPool();
    void updateUniformBuffers();
    void recordCommandBuffer();

    void prepare() override;
    void render() override {
        updateUniformBuffers();
        recordCommandBuffer();
        submitMainCommandBuffer();
        presentMainCommandBufferFrame();
    }

    // --- Geo & texture Resources ---
    glm::vec3 lightPos{-739.189,708.448,708.448};
    Resources resources{};


    // --- Geo & texture Resources ---
    VkDescriptorPool descPool{};
    std::unique_ptr<InstancePass> instancePass;
    std::unique_ptr<TerrainPassV2> terrainPass;


};
LLVK_NAMESPACE_END

