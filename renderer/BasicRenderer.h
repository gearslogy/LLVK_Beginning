//
// Created by lp on 2024/7/9.
//

#pragma once

#include "VulkanRenderer.h"
#include "GeoVertexDescriptions.h"
#include "DescriptorManager.h"
#include "BufferManager.h"
#include "SimplePipeline.h"
LLVK_NAMESPACE_BEGIN
struct BasicRenderer : public VulkanRenderer{
    BasicRenderer(): VulkanRenderer(){}

    void cleanupObjects() override;
    void loadTexture();
    void loadModel();
    void setupDescriptors();
    void preparePipelines();
    void updateUniformBuffer();
    void bindResources();

    void recordCommandBuffer();

    void prepare() override {
        bindResources();
        loadTexture();
        loadModel();
        setupDescriptors();
        preparePipelines();
    }
    void render() override {
        updateUniformBuffer();
        recordCommandBuffer();
        submitMainCommandBuffer();
        presentMainCommandBufferFrame();
    }

    ObjLoader simpleObjLoader{};
    BufferManager geometryBufferManager{};
    DescriptorManager simpleDescriptorManager{};
    SimplePipeline simplePipeline{};
};

LLVK_NAMESPACE_END
