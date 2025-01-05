//
// Created by 15 on 1/5/2025.
//

#pragma once

#include "VulkanRenderer.h"
#include "EpicRendererDefines.hpp"
LLVK_NAMESPACE_BEGIN
EPIC_NAMESSPACE_BEGIN
class EpicRenderer : public VulkanRenderer {
public:
    // desc set
    struct {// PVMIUBO ubo data
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 model;
    }uboMVPData{};


private:

};
EPIC_NAMES_SPACE_END
LLVK_NAMESPACE_END


