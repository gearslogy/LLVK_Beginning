//
// Created by liuyangping on 2025/3/24.
//

#pragma once

#include "LLVK_SYS.hpp"
#include <array>
#include <LLVK_VmaBuffer.h>
#include "LLVK_GeometryLoaderV2.hpp"
#include "renderer/public/CustomVertexFormat.hpp"// 这个之前必须include LLVK_GeometryLoaderV2.hpp 需要特化


// UBO
LLVK_NAMESPACE_BEGIN
namespace subpass{
    using FramedUBO = std::array<VmaUBOBuffer, MAX_FRAMES_IN_FLIGHT>;
    using FramedSet = std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>;
    using FramedSSBO = std::array<VmaSSBOBuffer, MAX_FRAMES_IN_FLIGHT>;


    struct GBufferAttachments{
        VmaAttachment albedo{};
        VmaAttachment NRM{};     // normal rough metallic
        VmaAttachment V{};       // velocity buffer
        VmaAttachment P{};       // P
        int width{};
        int height{};
    };

    //generally binding = 0
    struct GlobalUBO{
        glm::mat4 proj;
        glm::mat4 view;

        glm::mat4 preProj;
        glm::mat4 preView;

        glm::vec4 cameraPosition;
    };

    // for every render geometry
    struct xform {
        glm::mat4 model;
        glm::mat4 preModel;
    };

    inline constexpr std::array<VkVertexInputAttributeDescription, 4> attribsDesc{
            {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VTXFmt_P_N_T_UV0, P)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VTXFmt_P_N_T_UV0, N)},
                {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VTXFmt_P_N_T_UV0, T)},
                {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VTXFmt_P_N_T_UV0, uv0)},
            }
    };
    inline constexpr VkVertexInputBindingDescription vertexBinding{0, sizeof(VTXFmt_P_N_T_UV0), VK_VERTEX_INPUT_RATE_VERTEX};
    inline constexpr std::array bindingsDesc{vertexBinding};

}


LLVK_NAMESPACE_END
