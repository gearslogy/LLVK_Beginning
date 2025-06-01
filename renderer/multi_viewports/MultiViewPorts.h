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

struct MultiViewPortsShadowPass;
class MultiViewPorts : public VulkanRenderer{
public:
    MultiViewPorts();
    ~MultiViewPorts() override;
    static constexpr uint32_t VIEWPORTS_NUM = 2;
    static constexpr uint32_t push_constant_stage = VK_SHADER_STAGE_ALL;
    struct UBO {
        glm::mat4 proj[VIEWPORTS_NUM];
        glm::mat4 view[VIEWPORTS_NUM];
        glm::vec4 camPos[VIEWPORTS_NUM];
        glm::vec4 lightPos;
    }ubo{};

    struct Geometry{
        using vertex_t = VTXFmt_P_N_T_UV0;
        GLTFLoaderV2::Loader<vertex_t> geoLoader;
        VmaUBOTexture diff; // or D_Alpha
        VmaUBOTexture nrm; // normal rough metallic
        HLP::FramedSet sets;
        std::string name; // asset name for debug
        HLP::xform xform{};
        void cleanup() {
            diff.cleanup();
            nrm.cleanup();
        }
    };

public:
    void prepare() override;
    void render() override;
    void cleanupObjects() override;
    void drawObjects() const;
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
    std::unique_ptr<MultiViewPortsShadowPass> shadowPass;
};
LLVK_NAMESPACE_END


#endif //MULTIVIEWPORTS_H
