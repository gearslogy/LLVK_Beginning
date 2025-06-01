//
// Created by liuya on 4/23/2025.
//

#pragma once
#include <LLVK_Descriptor.hpp>
#include <LLVK_SYS.hpp>
#include <LLVK_VmaBuffer.h>
#include "LLVK_GeometryLoaderV2.hpp"
#include "CustomVertexFormat.hpp"
LLVK_NAMESPACE_BEGIN
namespace HLP {
    inline void createSimpleDescPool(const VkDevice &device, VkDescriptorPool &descPool) {
        std::array<VkDescriptorPoolSize, 4> poolSizes  = {{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 40 * MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 40 * MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 40 * MAX_FRAMES_IN_FLIGHT},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 40 * MAX_FRAMES_IN_FLIGHT}
        }};
        VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 40 * MAX_FRAMES_IN_FLIGHT); //
        createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
        if (auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool); result!=VK_SUCCESS) throw std::runtime_error("Failed to create descriptor pool!");
    }

    // types
    using FramedUBO = std::array<VmaUBOBuffer, MAX_FRAMES_IN_FLIGHT>;
    using FramedSet = std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>;
    using FramedSSBO = std::array<VmaSSBOBuffer, MAX_FRAMES_IN_FLIGHT>;

    // for every render geometry
    struct xform {
        glm::mat4 model{1.0};
        glm::mat4 preModel{1.0};
    };

    namespace VTXAttrib{
        inline constexpr std::array<VkVertexInputAttributeDescription, 4> VTXFmt_P_N_T_UV0_AttribsDesc{
                {
                        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, //offsetof() can not use here
                        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
                        {2, 0, VK_FORMAT_R32G32B32_SFLOAT, 24},
                        {3, 0, VK_FORMAT_R32G32_SFLOAT, 36},
                }
        };
        inline constexpr VkVertexInputBindingDescription VTXFmt_P_N_T_UV0_VertexBinding{0, sizeof(VTXFmt_P_N_T_UV0), VK_VERTEX_INPUT_RATE_VERTEX};
        inline constexpr std::array VTXFmt_P_N_T_UV0_BindingsDesc{VTXFmt_P_N_T_UV0_VertexBinding};
    }
}

LLVK_NAMESPACE_END
