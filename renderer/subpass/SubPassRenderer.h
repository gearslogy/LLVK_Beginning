//
// Created by liuyangping on 2025/2/26.
//
/*
 *
 * GBUFFER(swapchain Albedo P N depth) types:(rgb8_unorm rgb8_unorm rgba16_SFLOAT rgba16_SFLOAT)
 *
 * swapchain
 * Albedo
 * N
 * depth
 * vBuffer
 *
 */
#pragma once
#include <LLVK_UT_Pipeline.hpp>
#include "VulkanRenderer.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_VmaBuffer.h"


LLVK_NAMESPACE_BEGIN
class SubPassRenderer : public VulkanRenderer {
protected:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    void swapChainResize() override;
private:
    struct {
        glm::mat4 proj{};
        glm::mat4 view{};
        glm::mat4 model{};
        glm::vec4 camPos{};
    }uboData;



};


LLVK_NAMESPACE_END