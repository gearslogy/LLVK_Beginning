//
// Created by liuya on 8/16/2024.
//

#pragma once

#include "VulkanRenderer.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN

struct IndirectDrawPass;
struct TerrainPassV2;
struct IndirectResources {
    void loading();
    void cleanup();

    VulkanRenderer *pRenderer{nullptr};
    IndirectDrawPass *pIndirectDrawPass{nullptr};
    TerrainPassV2 *pTerrainPass{nullptr};
    struct  {
        GLTFLoader terrain{};// only use part0
        GLTFLoader grass{};
        GLTFLoader flower{};
        GLTFLoader tree{}; // include grass/
        VmaSimpleGeometryBufferManager geoBufferManager{};
    }geos;

    struct {
        // simulate terrain
        VmaUBOKTX2Texture albedoArray{};
        VmaUBOKTX2Texture normalArray{};
        VmaUBOKTX2Texture ordpArray{};
        VmaUBOKTX2Texture mask{};
    }terrainTextures;

    VmaUBOKTX2Texture foliageD_array;                    // albedo: grass flower tree_leaves tree_branch tree_root
    VmaUBOKTX2Texture foliageN_array;                    // N: grass flower tree_leaves tree_branch tree_root
    VmaUBOKTX2Texture foliageM_Array;                    // M:grass flower tree_leaves tree_branch tree_root ( R:roughness G:metalness B:AO A:unknown )


    struct {
        CombinedGLTFPart combinedGeometry;
        VmaSimpleGeometryBufferManager bufferManager{};
    }combinedObject;


    VkSampler colorSampler{};
private:
    void loadTerrain();
    void loadFoliage();
};



struct IndirectRenderer final : VulkanRenderer {
    IndirectRenderer();
    ~IndirectRenderer() override;
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
    IndirectResources resources{};


    // --- Geo & texture Resources ---
    VkDescriptorPool descPool{};
    std::unique_ptr<IndirectDrawPass> indirectDrawPass;
    std::unique_ptr<TerrainPassV2> terrainPass;


};
LLVK_NAMESPACE_END

