//
// Created by liuyangping on 2024/6/26.
//

#ifndef PUSHCONSTANT_H
#define PUSHCONSTANT_H
#include <vulkan/vulkan.h>
#include <array>
#include "Utils.h"

// only test
struct PushVertexStageData{
    static constexpr uint32_t shader_stage_type = VK_SHADER_STAGE_VERTEX_BIT;
    float P_xOffset{};
    float P_yOffset{};
    float P_zOffset{};
    float P_wOffset{};
};

// only test
struct PushFragmentStageData {
    static constexpr uint32_t shader_stage_type = VK_SHADER_STAGE_FRAGMENT_BIT;
    float hasDiffTex{};
    float hasRoughTex{};
    float hasUv{};
    float hasCd{};
};

struct PushConstant {
    static constexpr uint32_t count = 2;
    static constexpr VkPushConstantRange pushRanges[count] = {
        {VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(PushVertexStageData)},
        {VK_SHADER_STAGE_FRAGMENT_BIT,sizeof(PushVertexStageData),sizeof(PushFragmentStageData)},
    };

    // update one
    template<uint32_t PushDataStage>
    static void update(uint32_t frame, auto &&fragmentUpdateCallback) {
        if constexpr (PushDataStage == VK_SHADER_STAGE_VERTEX_BIT)
            fragmentUpdateCallback(vertexPushConstants[frame]);
        else if constexpr (PushDataStage == VK_SHADER_STAGE_FRAGMENT_BIT)
            fragmentUpdateCallback(fragmentPushConstants[frame]);
        else static_assert(ALWAYS_TRUE, "ERROR unsupport vk format");
    }
    // update all
    static void update(uint32_t frame, auto &&vertexUpdateCallback, auto &&fragmentUpdateCallBack) {
        update<VK_SHADER_STAGE_VERTEX_BIT>(frame,vertexUpdateCallback);
        update<VK_SHADER_STAGE_FRAGMENT_BIT>(frame,fragmentUpdateCallBack);
    }
    static inline std::array<PushVertexStageData,MAX_FRAMES_IN_FLIGHT> vertexPushConstants{};
    static inline std::array<PushFragmentStageData,MAX_FRAMES_IN_FLIGHT> fragmentPushConstants{};
};

#endif //PUSHCONSTANT_H
