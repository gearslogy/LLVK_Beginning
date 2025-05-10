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
    static constexpr uint32_t VIEWPORTS_NUM = 2;
    struct UBO {
        glm::mat4 proj[VIEWPORTS_NUM];
        glm::mat4 modelView[VIEWPORTS_NUM];
        glm::vec4 lightPos;
    }ubo;
    struct Geometry{
        using vertex_t = VTXFmt_P_N_T_UV0;
        GLTFLoaderV2::Loader<vertex_t> geoLoader;
        VmaUBOTexture diff; // or D_Alpha
        VmaUBOTexture nrm; // normal rough metallic
        HLP::FramedSet sets;
        std::string name; // asset name for debug
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
    void prepareUBOs();
    void preparePipeline();
    void prepareDescriptorSets();
    void updateUBOs();
private:
    VkDescriptorPool descPool{};
    UT_GraphicsPipelinePSOs pso{};
    void recordCommandBuffer();
    void loadGeometry();
    Geometry grid{};
    Geometry treeLeaves{};
    Geometry treeTrunk{};
    VmaSimpleGeometryBufferManager geomManager{};
    VkSampler colorSampler{};
    VkDescriptorSetLayout setLayout{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
    HLP::FramedUBO uboBuffers{};
    Camera leftCam{};
    Camera rightCam{};
    Camera topCam{};
};
LLVK_NAMESPACE_END


#endif //MULTIVIEWPORTS_H
