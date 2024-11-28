//
// Created by liuya on 11/8/2024.
//

#include "CSMRenderer.h"

#include <LLVK_Descriptor.hpp>
#include <LLVK_RenderPass.hpp>

#include "CSMPass.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "LLVK_RenderPass.hpp"
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
    geos.geo_36.load("content/scene/csm/resources/gpu_models/39_MedBuilding.gltf");
    colorSampler =  FnImage::createImageSampler(phyDevice, device);
    textures.d_ground.create("content/scene/csm/resources/gpu_textures/ground_gpu_D.ktx2",colorSampler);
    textures.d_tex_29.create("content/scene/csm/resources/gpu_textures/29_WatchTower_gpu_D.ktx2",colorSampler);
    textures.d_tex_35.create("content/scene/csm/resources/gpu_textures/35_MedBuilding_gpu_D.ktx2",colorSampler);
    textures.d_tex_36.create("content/scene/csm/resources/gpu_textures/36_MedBuilding_gpu_D.ktx2",colorSampler);
    textures.d_tex_39.create("content/scene/csm/resources/gpu_textures/39_MedBuilding_gpu_D.ktx2",colorSampler);
}

CSMRenderer::ResourceManager::~ResourceManager() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geos.geometryBufferManager,textures.d_ground, textures.d_tex_29,
     textures.d_tex_35, textures.d_tex_36,textures.d_tex_39);
    UT_Fn::cleanup_sampler(device, colorSampler);

}

void CSMRenderer::prepare() {
    resourceManager.pRenderer = this;
    resourceManager.loading();
}
void CSMRenderer::render() {

}
void CSMRenderer::cleanupObjects() {
    const auto &device = getMainDevice().logicalDevice;
    UT_Fn::cleanup_sampler(device, depthSampler);
    depthAttachment.cleanup();
    UT_Fn::cleanup_render_pass(device, depthRenderPass);
    UT_Fn::cleanup_framebuffer(device,depthFramebuffer);
    auto cleanFramedUBOs = [](auto &&... framedUBO) {
        (UT_Fn::cleanup_range_resources(framedUBO), ...);
    };
    cleanFramedUBOs(uboGround, ubo29,ubo35,ubo36,ubo39);
    vkDestroyDescriptorPool(device, descPool, nullptr);
}
void CSMRenderer::prepareOffscreenDepth() {
    const auto &device = getMainDevice().logicalDevice;
    depthSampler = FnImage::createDepthSampler(device);
    setRequiredObjectsByRenderer(this, depthAttachment);
    // render target
    depthAttachment.create2dArrayDepth32(2048,2048, CASCADE_COUNT,depthSampler);
    // render pass
    prepareDepthRenderPass();
    prepareFramebuffer();
}
void CSMRenderer::prepareDepthRenderPass() {
    auto depthATM = FnRenderPass::depthAttachmentDescription(VK_FORMAT_R32_SFLOAT);
    auto depthATM_Ref = FnRenderPass::attachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // before renderpass
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;    // before stage
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;   // after stage
    dependency.srcAccessMask = 0; // before access 首次写入，不需要等待之前的访问
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ; // after access
    auto subpassDesc = FnRenderPass::subpassDescription();
    subpassDesc.colorAttachmentCount = 0;
    subpassDesc.pDepthStencilAttachment = &depthATM_Ref;

    const std::array attachments = { depthATM};
    auto renderPassCIO = FnRenderPass::renderPassCreateInfo(attachments);
    renderPassCIO.subpassCount = 1;
    renderPassCIO.pSubpasses = &subpassDesc;
    renderPassCIO.dependencyCount = 1;
    renderPassCIO.pDependencies = &dependency;
    const auto device = getMainDevice().logicalDevice;
    const auto ret = vkCreateRenderPass(device, &renderPassCIO, nullptr, &depthRenderPass);
    if(ret != VK_SUCCESS) throw std::runtime_error{"ERROR"};
}
void CSMRenderer::prepareFramebuffer() {
    const auto &device = getMainDevice().logicalDevice;
    std::array<VkImageView, 1> attachments{
        depthAttachment.view
    };
    auto fbCIO  = FnRenderPass::framebufferCreateInfo(shadow_map_size,shadow_map_size, depthRenderPass,attachments);
    fbCIO.layers = CASCADE_COUNT;
    if (vkCreateFramebuffer(device, &fbCIO, nullptr, &depthFramebuffer)!= VK_SUCCESS) throw std::runtime_error{"ERROR"};

}

void CSMRenderer::prepareUBOAndDesc() {
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
    uboData.model = glm::translate(identity,{-24.0f, 0.0f , 1.5f}) * glm::rotate(identity,-28.6f, {0,1,0}) ;
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

    // --- set layout and sets allocation. 0:UBO 1:CIS 2:CIS(sample depth map)
    using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO, MetaDesc::CIS, MetaDesc::CIS>;
    using descPos = MetaDesc::desc_binding_position_t<0,1,2>;
    using descBindingUsage = MetaDesc::desc_binding_usage_t<VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT>;
    constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
    const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
    if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&sceneDescLayout) != VK_SUCCESS) throw std::runtime_error("error create set layout");

    std::array<VkDescriptorSetLayout,2> layouts = {sceneDescLayout, sceneDescLayout};
    auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, setGround.data());
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, set29.data());
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, set35.data());
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, set36.data());
    UT_Fn::invoke_and_check("create scene sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, set39.data());

    // update sets
    auto updateSets= [&device](const UBOFramedBuffers &framedUbo, const SetsFramed&framedSet, const auto &... textures) {
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                std::array<VkWriteDescriptorSet, 1 + sizeof...(textures)> writes = {
                    FnDescriptor::writeDescriptorSet(framedSet[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &framedUbo[i].descBufferInfo),
                    FnDescriptor::writeDescriptorSet(framedSet[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        I + 1, &std::get<I>(std::forward_as_tuple(textures...)).descImageInfo)...
                };
                vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
            }
        }(std::make_index_sequence<sizeof...(textures)>{});
    };
    updateSets(uboGround, setGround, resourceManager.textures.d_ground, depthAttachment);
    updateSets(ubo29, set29, resourceManager.textures.d_tex_29,depthAttachment);
    updateSets(ubo35, set35, resourceManager.textures.d_tex_35,depthAttachment);
    updateSets(ubo36, set36, resourceManager.textures.d_tex_36,depthAttachment);
    updateSets(ubo39, set39, resourceManager.textures.d_tex_39,depthAttachment);
}



LLVK_NAMESPACE_END
