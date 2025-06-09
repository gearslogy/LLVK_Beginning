//
// Created by liuya on 6/9/2025.
//

#include "MS_TriangleRenderer.h"
#include "renderer/public/Helper.hpp"
LLVK_NAMESPACE_BEGIN
void MS_TriangleRenderer::cleanupObjects(){
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    UT_Fn::cleanup_descriptor_pool(device, descPool);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_pipeline(device, pipeline);
}


void MS_TriangleRenderer::prepare(){
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    HLP::createSimpleDescPool(device, descPool);
}

void MS_TriangleRenderer::render(){

}


LLVK_NAMESPACE_END