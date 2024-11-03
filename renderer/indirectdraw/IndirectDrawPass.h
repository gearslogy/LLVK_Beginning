//
// Created by lp on 2024/9/19.
//

#pragma once
#include <LLVK_UT_Pipeline.hpp>
#include <libs/json.hpp>
#include "LLVK_GeomtryLoader.h"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
class VulkanRenderer;


struct IndirectDrawPass {
    struct InstanceData {
        glm::vec3 P;
        glm::vec4 orient;
        float pscale;
        uint32_t texId;
    };

    struct JsonPointsParser {
        explicit JsonPointsParser(const std::string &path);
        nlohmann::json jsHandle;
        std::vector< InstanceData> instanceData;
    };

    IndirectDrawPass(const VulkanRenderer* renderer, const VkDescriptorPool *descPool);
    void cleanup();
    void prepare();
    void updateUniformBuffers(const glm::mat4 &depthMVP, const glm::vec4 &lightPos);
    void recordCommandBuffer();



    VkBuffer instanceBuffer;
    VkBuffer verticesBuffer{};
    VkBuffer indicesBuffer{};
    VkBuffer indirectCommandBuffer{};
    uint32_t indirectDrawCount{};
    std::vector<const IVmaUBOTexture *> pTextures;
private:
    void prepareUniformBuffers();
    void prepareDescriptorSets();
    void preparePipelines();



    UT_GraphicsPipelinePSOs pipelinePSOs;

    const VulkanRenderer * pRenderer{VK_NULL_HANDLE};      // required object at ctor
    const VkDescriptorPool *pDescriptorPool{VK_NULL_HANDLE}; // required object at ctor

    struct UniformDataScene {
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model;
        glm::mat4 depthBiasMVP;
        glm::vec4 lightPos;
        // Used for depth map visualization
        float zNear{0.1};
        float zFar{1000.0};
    } uniformDataScene;

    std::array<VmaUBOBuffer,MAX_FRAMES_IN_FLIGHT> uboBuffers;
    std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setUBOs;       // set=0
    std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> setTextures;   // set=1


    VkDescriptorSetLayout uboDescSetLayout{};
    VkDescriptorSetLayout textureDescSetLayout{};
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline{};
};






LLVK_NAMESPACE_END


