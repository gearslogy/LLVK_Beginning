//
// Created by liuya on 11/23/2024.
//

#pragma once
#include <LLVK_SYS.hpp>
#include <vulkan/vulkan_core.h>
LLVK_NAMESPACE_BEGIN

namespace DualPassGlobal {
    constexpr auto colorAttachmentFormat = VK_FORMAT_R8G8B8A8_UNORM;
    constexpr auto depthStencilAttachmentFormat = VK_FORMAT_D32_SFLOAT;

}
LLVK_NAMESPACE_END