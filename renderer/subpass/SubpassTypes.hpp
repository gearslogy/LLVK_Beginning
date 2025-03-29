//
// Created by liuyangping on 2025/3/24.
//

#pragma once

#include "LLVK_SYS.hpp"
#include <array>
#include <LLVK_VmaBuffer.h>
#include "renderer/public/UT_CustomRenderer.hpp"

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

}


LLVK_NAMESPACE_END
