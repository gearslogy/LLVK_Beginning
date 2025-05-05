//
// Created by liuya on 4/8/2025.
//

#ifndef MULTIVIEWPORTS_H
#define MULTIVIEWPORTS_H

#include <LLVK_GeometryLoaderV2.hpp>
#include <LLVK_UT_Pipeline.hpp>
#include "VulkanRenderer.h"
#include "renderer/public/Helper.hpp"


LLVK_NAMESPACE_BEGIN
class MultiViewPorts : public VulkanRenderer{
public:
    struct UBO {
        glm::mat4 proj[4]; // left right top bottom
        glm::mat4 modelView[4];
        glm::vec4 lightPos;
    }ubo;
    struct Geometry{
        using vertex_t = VTXFmt_P_N_T_UV0;
        GLTFLoaderV2::Loader<vertex_t> geoLoader;
        VmaUBOTexture diff; // or D_Alpha
        VmaUBOTexture nrm; // normal rough metallic

        void cleanup() {
            diff.cleanup();
            nrm.cleanup();
        }
    };

public:
    void prepare() override;
    void render() override;
    void cleanupObjects() override;
private:
    void preparePipeline();
    void prepareDescriptorSets();
private:
    VkDescriptorPool descPool{};
    UT_GraphicsPipelinePSOs pso{};
    void recordCommandBuffer();
    void loadGeometry();
    Geometry grid{};
    Geometry tree{};
    VmaSimpleGeometryBufferManager geomManager{};
    VkSampler colorSampler{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};



};
LLVK_NAMESPACE_END


#endif //MULTIVIEWPORTS_H
