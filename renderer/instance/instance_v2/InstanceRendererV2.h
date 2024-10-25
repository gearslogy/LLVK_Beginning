//
// Created by liuya on 8/16/2024.
//

#ifndef INSTANCE_H
#define INSTANCE_H


#include "VulkanRenderer.h"
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN

struct Resources {
    void loading();
    void cleanup();

    VulkanRenderer *pRenderer{nullptr};

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



class InstancedObjectPass;
struct InstanceRendererV2 : public VulkanRenderer {
    InstanceRendererV2();
    ~InstanceRendererV2() override;
    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
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
    std::unique_ptr<InstancedObjectPass> instancePass;


};
LLVK_NAMESPACE_END


#endif //INSTANCE_H
