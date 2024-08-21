//
// Created by liuya on 8/16/2024.
//

#ifndef INSTANCE_H
#define INSTANCE_H


#include "VulkanRenderer.h"
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
struct InstanceRenderer : public VulkanRenderer {

    InstanceRenderer(): VulkanRenderer(){}

    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
    void setupDescriptors();
    void preparePipelines();
    void prepareUniformBuffers();
    void updateUniformBuffers();
    void bindResources();

    void recordCommandBuffer();

    void prepare() override {
        bindResources();
        loadTexture();
        loadModel();
        prepareUniformBuffers();
        setupDescriptors();
        preparePipelines();
    }
    void render() override {
        updateUniformBuffers();
        recordCommandBuffer();
    }
    struct  {
        GLTFLoader plantGeo; // only use part0
        GLTFLoader groundGeo;// only use part0
        VmaSimpleGeometryBufferManager bufferManager;
    }Geos;


};
LLVK_NAMESPACE_END


#endif //INSTANCE_H
