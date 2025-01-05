//
// Created by 15 on 1/5/2025.
//

#ifndef TERRAINPASS_H
#define TERRAINPASS_H


#include "renderer/epic/EpicRendererDefines.hpp"

LLVK_NAMESPACE_BEGIN
EPIC_NAMESSPACE_BEGIN
class EpicRenderer;
struct TerrainPass {
    EpicRenderer *pRenderer{};
    void prepare();
    void cleanup();

    VkDescriptorSetLayout setLayout{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};
};
EPIC_NAMES_SPACE_END
LLVK_NAMESPACE_END

#endif //TERRAINPASS_H
