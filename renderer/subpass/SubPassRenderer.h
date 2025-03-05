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
struct SubPassResource;
class SubPassRenderer : public VulkanRenderer {
protected:
    ~SubPassRenderer() override = default;
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
    void createFramebuffers() override;

private:
    struct {
        glm::mat4 proj{};
        glm::mat4 view{};
        glm::mat4 model{};
        glm::vec4 camPos{};
    }uboData;

    void createAttachments();
    std::unique_ptr<SubPassResource> resourceLoader;


    VkSampler colorSampler{};
    struct {
        VmaAttachment albedo{};
        VmaAttachment NRM{};
    }attachments;


private:
    VkDevice usedDevice{};
    VkPhysicalDevice usedPhyDevice{};
};


LLVK_NAMESPACE_END