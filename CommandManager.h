//
// Created by star on 4/28/2024.
//

#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <vector>
#include "PushConstant.h"
#include <iostream>
struct FnCommand {
    // single time command
    static VkCommandBuffer beginSingleTimeCommand(VkDevice device, VkCommandPool pool);

    static void endSingleTimeCommand(VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer cmdBuf);

    static VkCommandPool createCommandPool(VkDevice device, uint32_t queueFamilyIndex,
                                           const VkCommandPoolCreateInfo *poolCreateInfo = nullptr);


    // when command begin
    struct RenderCommandBeginInfo {
        VkCommandBufferBeginInfo commandBufferBeginInfo;
        VkRenderPassBeginInfo renderPassBeginInfo;
    };
    static RenderCommandBeginInfo createCommandBufferBeginInfo(const VkFramebuffer &framebuffer,
                                                         const VkRenderPass &renderpass,
                                                         const VkExtent2D *swapChainExtent,
                                                         const std::vector<VkClearValue> &clearValues);

};


/* struct for building command:vkCmdBindVertexBuffers
VkBuffer vertexBuffers[] = {};
VkDeviceSize offsets[] = {0}; // 0从每个buffer的起始位置开始读取
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
 */
struct CmdBindVertexBuffers {
    std::vector<VkBuffer>     vertexBuffers;
    std::vector<VkDeviceSize> offsets;
    uint32_t firstBinding;
    uint32_t bindingCount;
    size_t vertexCount; // only use vkCmdDraw
};
struct CmdBindIndexBuffer {
    VkBuffer indexBuffer;
    VkDeviceSize offset;
    VkIndexType indexType;
    size_t indexCount;  // only use vkCmdDrawIndexed
};

struct CommandManager {
    VkDevice bindLogicDevice{};
    VkPhysicalDevice bindPhysicalDevice{};
    VkSurfaceKHR bindSurface{};
    const std::vector<VkFramebuffer> *bindSwapChainFramebuffers;
    const VkExtent2D *bindSwapChainExtent;
    VkRenderPass bindRenderPass;
    VkPipeline bindPipeline; // which pipeline to rendering
    VkPipelineLayout bindPipeLineLayout;
    CmdBindVertexBuffers bindVertexBuffers;
    CmdBindIndexBuffer bindIndexBuffer;
    const std::vector<VkDescriptorSet> *bindDescriptorSets;
    const int *bindCurrentFrame;
    // created
    VkCommandPool graphicsCommandPool{};
    std::vector<VkCommandBuffer> commandBuffers;// Resize command buffer count to have one for each framebuffer
    void cleanup();
    void recordCommand(VkCommandBuffer cmdBuffer, uint32_t imageIndex);

    // call in every frame
    void recordCommandWithGeometry(const auto &geometry, VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
        static_assert(requires { geometry.vertices; });
        static_assert(requires { geometry.indices; });
        // select framebuffer
        std::vector<VkClearValue> clearValues(2);
        clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        const VkFramebuffer &framebuffer = (*bindSwapChainFramebuffers)[imageIndex];
        auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(framebuffer,
            bindRenderPass,
            bindSwapChainExtent,clearValues);

        auto result = vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
        if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
        vkCmdBeginRenderPass(cmdBuffer, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS ,bindPipeline);
        setViewPortAndScissor(cmdBuffer);
        // 更新当前帧的 Push Constants
        //std::cout << "push range1:" << 0 << " " << sizeof(PushVertexStageData) << std::endl;
        vkCmdPushConstants(cmdBuffer,
            bindPipeLineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushVertexStageData),
            &PushConstant::vertexPushConstants[*bindCurrentFrame]);
        //std::cout << "push range2:" << sizeof(PushFragmentStageData) << " " << sizeof(PushVertexStageData) << std::endl;
        vkCmdPushConstants(cmdBuffer,
            bindPipeLineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(PushVertexStageData), // 偏移量为 Vertex Push Constant 的大小
            sizeof(PushFragmentStageData),
            &PushConstant::fragmentPushConstants[*bindCurrentFrame]);

        renderIndicesGeometryCommand(geometry, cmdBuffer, [cmdBuffer,this]() {
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    bindPipeLineLayout, 0, 2,
                    &(*bindDescriptorSets)[*bindCurrentFrame * 2],
                    0, nullptr);
        });
        vkCmdEndRenderPass(cmdBuffer);
        if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

    }
    void createGraphicsCommandPool();
    void createCommandBuffers();

    void setViewPortAndScissor(VkCommandBuffer cmdBuffer);


};



#endif //COMMANDBUFFER_H
