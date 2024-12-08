//
// Created by liuya on 11/8/2024.
//

#include "CSMRenderer.h"

#include <LLVK_Descriptor.hpp>
#include <LLVK_RenderPass.hpp>
#include <LLVK_UT_VmaBuffer.hpp>

#include "CSMDepthPass.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "LLVK_RenderPass.hpp"
#include "CSMScenePass.h"
#include "CSMDepthPass.h"
#include "renderer/public/GeometryContainers.h"
LLVK_NAMESPACE_BEGIN
void CSMRenderer::ResourceManager::loading() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &phyDevice = pRenderer->getMainDevice().physicalDevice;
    setRequiredObjectsByRenderer(pRenderer, geos.geometryBufferManager);
    setRequiredObjectsByRenderer(pRenderer, textures.d_tex_29, textures.d_tex_35,
        textures.d_tex_39,textures.d_tex_36, textures.d_ground);
    geos.ground.load("content/scene/csm/resources/gpu_models/ground.gltf");
    geos.geo_29.load("content/scene/csm/resources/gpu_models/29_WatchTower.gltf");
    geos.geo_35.load("content/scene/csm/resources/gpu_models/35_MedBuilding.gltf");
    geos.geo_36.load("content/scene/csm/resources/gpu_models/36_MedBuilding.gltf");
    geos.geo_39.load("content/scene/csm/resources/gpu_models/39_MedBuilding.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.ground, geos.geometryBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.geo_29, geos.geometryBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.geo_35, geos.geometryBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.geo_36, geos.geometryBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(geos.geo_39, geos.geometryBufferManager);


    colorSampler =  FnImage::createImageSampler(phyDevice, device);
    textures.d_ground.create("content/scene/csm/resources/gpu_textures/ground_gpu_D.ktx2",colorSampler);
    textures.d_tex_29.create("content/scene/csm/resources/gpu_textures/29_WatchTower_gpu_D.ktx2",colorSampler);
    textures.d_tex_35.create("content/scene/csm/resources/gpu_textures/35_MedBuilding_gpu_D.ktx2",colorSampler);
    textures.d_tex_36.create("content/scene/csm/resources/gpu_textures/36_MedBuilding_gpu_D.ktx2",colorSampler);
    textures.d_tex_39.create("content/scene/csm/resources/gpu_textures/39_MedBuilding_gpu_D.ktx2",colorSampler);
}

void CSMRenderer::ResourceManager::cleanup() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geos.geometryBufferManager,textures.d_ground, textures.d_tex_29,
     textures.d_tex_35, textures.d_tex_36,textures.d_tex_39);
    UT_Fn::cleanup_sampler(device, colorSampler);

}

CSMRenderer::CSMRenderer() {
    scenePass = std::make_unique<CSMScenePass>(this);
    depthPass = std::make_unique<CSMDepthPass>(this);
}
CSMRenderer::~CSMRenderer() = default;


void CSMRenderer::prepare() {
    resourceManager.pRenderer = this;
    resourceManager.loading();
    mainCamera.setRotation({-10.582830028019421, -12.201134395235595, -1.7394687750417736e-05});
    mainCamera.mPosition = {-7.345118601417127, 16.530968869105806, 69.05524044127405};
    mainCamera.mMoveSpeed = 10;
    mainCamera.updateCameraVectors();
    preparePVMIUBOAndSets();
    depthPass->prepare();
    scenePass->prepare();
    // when depth ready. we can update sets. because need the attachment, and ubo-geom info
    updateSets();

}
void CSMRenderer::render() {
    // update light
    // update cascade
    // update ubo
    updateUBO();
    depthPass->update();
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin dual pass command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    depthPass->recordCommandBuffer();
    scenePass->recordCommandBuffer();

    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}
void CSMRenderer::cleanupObjects() {
    const auto &device = getMainDevice().logicalDevice;
    auto cleanFramedUBOs = [](auto &&... framedUBO) {
        (UT_Fn::cleanup_range_resources(framedUBO), ...);
    };
    cleanFramedUBOs(uboGround, ubo29,ubo35,ubo36,ubo39);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    scenePass->cleanup();
    depthPass->cleanup();
    resourceManager.cleanup();
    UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
}

void CSMRenderer::preparePVMIUBOAndSets() {
    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR"};

    // ---- UBO create
    auto createFramedUBO = [this](UBOFramedBuffers &fbs) {
        for (auto &bf : fbs) {
            setRequiredObjectsByRenderer(this, bf);
            bf.createAndMapping(sizeof(uboData));
        }
    };
    auto createFramedUBOs = [createFramedUBO](auto & ... ubo) { (createFramedUBO(ubo), ...);};
    createFramedUBOs(uboGround, ubo29, ubo35, ubo36, ubo39);


    // --- set layout and sets allocation. 0:UBO(vs) 1:UBO(geom) 2:CIS 3:CIS(sample depth map)
    using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO, MetaDesc::UBO, MetaDesc::CIS, MetaDesc::CIS>; // MVP/geomUBO/color/depth
    using descPos = MetaDesc::desc_binding_position_t<0,1,2,3>;
    using descBindingUsage = MetaDesc::desc_binding_usage_t<VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_GEOMETRY_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT>;
    constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
    const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
    if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&descSetLayout) != VK_SUCCESS) throw std::runtime_error("error create set layout");

    std::array<VkDescriptorSetLayout,2> layouts = {descSetLayout, descSetLayout};
    auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, setGround.data());
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, set29.data());
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, set35.data());
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, set36.data());
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, set39.data());

    // pipeline layout
    const std::array setLayouts{descSetLayout}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device,
        &pipelineLayoutCIO,nullptr, &pipelineLayout );

}
void CSMRenderer::updateSets() {
    const auto &device = mainDevice.logicalDevice;
    namespace FnDesc = FnDescriptor;
    // update sets
    auto updateSets= [&device,this](const UBOFramedBuffers&mvp_i_framedUbo, const SetsFramed&framedSet, const auto &... textures) {
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                std::array<VkWriteDescriptorSet, 2 + sizeof...(textures)> writes = {
                    FnDesc::writeDescriptorSet(framedSet[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &mvp_i_framedUbo[i].descBufferInfo),          // scene model_view_proj_instance , used in VS shader
                    FnDesc::writeDescriptorSet(framedSet[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &depthPass->uboGeomBuffer[i].descBufferInfo), // used in geom shader. !IMPORTANT: depthPass->prepare()
                    FnDesc::writeDescriptorSet(framedSet[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        I + 2, &std::get<I>(std::forward_as_tuple(textures...)).descImageInfo)...
                };
                vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
            }
        }(std::make_index_sequence<sizeof...(textures)>{});
    };
    updateSets(uboGround, setGround, resourceManager.textures.d_ground,depthPass->renderTarget());
    updateSets(ubo29, set29, resourceManager.textures.d_tex_29,depthPass->renderTarget());
    updateSets(ubo35, set35, resourceManager.textures.d_tex_35,depthPass->renderTarget());
    updateSets(ubo36, set36, resourceManager.textures.d_tex_36,depthPass->renderTarget());
    updateSets(ubo39, set39, resourceManager.textures.d_tex_39,depthPass->renderTarget());
}


void CSMRenderer::updateUBO() {
    // ----update buffer. we do not update per every frame
    constexpr auto identity = glm::mat4(1.0f);
    auto [width, height] =  getSwapChainExtent();
    auto &&mainCamera = getMainCamera();
    const auto frame = getCurrentFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    uboData.model = glm::mat4(1.0f);

    auto copyDataToFramedBuffer = [](UBOFramedBuffers &fbs, auto &data) {
        for (auto &bf : fbs) {
            memcpy(bf.mapped, &data, sizeof(uboData));
        }
    };
    copyDataToFramedBuffer(uboGround,uboData); // direct to ground
    uboData.model = glm::translate(identity,{-3.5108f, 0.0f , -0.8984f}) * glm::rotate(identity,-180.6f, {0,1,0}) ;
    copyDataToFramedBuffer(ubo36,uboData);

    uboData.model = glm::translate(identity,{24.0f, 0.0f , 1.5f}) * glm::rotate(identity,-28.6f, {0,1,0}) ;
    copyDataToFramedBuffer(ubo39,uboData);

    uboData.model = glm::translate(identity,{7.066614747047424, 0.0, -47.99501860141754}) ;
    copyDataToFramedBuffer(ubo35,uboData);
    // instanced 29
    uboData.model = identity;
    uboData.instancesPositions[0] = {-2.7002276182174683, 0.0, 16.88442873954773,1.0f};
    uboData.instancesPositions[1] ={22.42972767353058, 0.0, 16.88442873954773,1.0f};
    uboData.instancesPositions[2] ={22.42972767353058, 0.0, -17.458569765090942,1.0f};
    uboData.instancesPositions[3] = {-4.5874022245407104, 0.0, -17.458569765090942,1.0f};
    copyDataToFramedBuffer(ubo29,uboData);
}

void CSMRenderer::renderGeometry(VkPipeline normalPipeline, VkPipeline instancePipeline) const {
    const auto &cmdBuf = getMainCommandBuffer();
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , normalPipeline);
    // render ground
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
      0, 1, &setGround[getCurrentFrame()] ,
      0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &resourceManager.geos.ground.parts[0]);

    // render 35
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
      0, 1, &set35[getCurrentFrame()] ,
      0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &resourceManager.geos.geo_35.parts[0]);
    // render 36
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
      0, 1, &set36[getCurrentFrame()] ,
      0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &resourceManager.geos.geo_36.parts[0]);
    // render 39
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
      0, 1, &set39[getCurrentFrame()] ,
      0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &resourceManager.geos.geo_39.parts[0]);

    // render instance geometry
    constexpr auto instanceCount = 4;
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , instancePipeline);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
       0, 1, &set29[getCurrentFrame()] ,
       0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &resourceManager.geos.geo_29.parts[0], instanceCount);
}


LLVK_NAMESPACE_END
