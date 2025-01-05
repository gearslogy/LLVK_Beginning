//
// Created by 15 on 1/5/2025.
//

#include "TerrainPass.h"
#include "LLVK_Utils.hpp"
#include "renderer/epic/EpicRenderer.h"
LLVK_NAMESPACE_BEGIN
    EPIC_NAMESSPACE_BEGIN

void TerrainPass::cleanup() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_descriptor_set_layout(device, setLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_pipeline(device, pipeline);
}

void TerrainPass::prepare() {

}




EPIC_NAMES_SPACE_END
LLVK_NAMESPACE_END