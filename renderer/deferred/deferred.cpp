//
// Created by liuya on 8/16/2024.
//

#include "deferred.h"
#include <ranges>
#include <algorithm>
#include <filesystem>

#include  "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
#include <vector>
#include <libs/magic_enum.hpp>

#include "Pipeline.hpp"
LLVK_NAMESPACE_BEGIN
defer::defer() {
     mainCamera.mPosition = {154.865,205.119,198.508};
     auto [width, height] = simpleSwapchain.swapChainExtent;
     mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
     mainCamera.mYaw = 243.6;
     mainCamera.mPitch = -43.99;
     mainCamera.mMoveSpeed = 20;
     mainCamera.updateCameraVectors();
}


void defer::cleanupObjects() {
     auto device = mainDevice.logicalDevice;
     uniformBuffers.composition.cleanup();
     uniformBuffers.mrt.cleanup();

     for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
          vkDestroySemaphore(device, mrtSemaphores[i], nullptr);
     }
     vkDestroyRenderPass(device, mrtFrameBuf.renderPass, nullptr);
     vkDestroySampler(device,colorSampler, nullptr);
     vkDestroyDescriptorPool(device, descPool, nullptr);
     UT_Fn::cleanup_descriptor_set_layout(device, geoDescriptorSets.setLayout0, geoDescriptorSets.setLayout1);
     UT_Fn::cleanup_descriptor_set_layout(device, compositionDescriptorSets.setLayout0, compositionDescriptorSets.setLayout1);
     UT_Fn::cleanup_pipeline_layout(device,pipelines.compositionLayout, pipelines.mrtLayout);
     UT_Fn::cleanup_pipeline(device,pipelines.mrt, pipelines.composition);
     for(auto &tex: UBOTextures.ground_textures)
          tex.cleanup();
     for(auto &tex: UBOTextures.skull_textures)
          tex.cleanup();
     cleanupMrtFramebuffer();
     simpleGeoBufferManager.cleanup();
}
void defer::loadModels() {
     ground_gltf.load("content/deferred/ground/ground.gltf");
     skull_gltf.load("content/deferred/skull/skull.gltf");
     simpleGeoBufferManager.requiredObjects.allocator = vmaAllocator;
     simpleGeoBufferManager.requiredObjects.device = mainDevice.logicalDevice;
     simpleGeoBufferManager.requiredObjects.queue = mainDevice.graphicsQueue;
     simpleGeoBufferManager.requiredObjects.commandPool = graphicsCommandPool;
     simpleGeoBufferManager.requiredObjects.physicalDevice = mainDevice.physicalDevice;
     UT_VmaBuffer::addGeometryToSimpleBufferManager(ground_gltf, simpleGeoBufferManager);
     UT_VmaBuffer::addGeometryToSimpleBufferManager(skull_gltf, simpleGeoBufferManager);
}
void defer::loadTextures() {
     colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
     for(auto &&t : UBOTextures.ground_textures)
          setRequiredObjects(t);
     for(auto &&t: UBOTextures.skull_textures)
          setRequiredObjects(t);

     namespace fs = std::filesystem;
     const fs::path root = "content/deferred";
     std::vector<std::string> names = {
          "albedo.jpg", "normal.jpg", "roughness.jpg", "displacement.jpg"
     };
     auto ground_root = root / "ground";
     auto skull_root = root / "skull";

     // read ground textures
     for(auto &&[i, tex] : UT_Fn::enumerate(UBOTextures.ground_textures) ){
          auto ground_file = ground_root / names[i];
          std::cout << ground_file << std::endl;
          if (not names[i].contains("normal") ) {
               tex.create(ground_file.generic_string(), colorSampler, VK_FORMAT_R8G8B8A8_SRGB );
          }
          else tex.create(ground_file.generic_string(), colorSampler, VK_FORMAT_R8G8B8A8_UNORM );

     }
     // read skull textures
     for(auto &&[i, tex] : UT_Fn::enumerate(UBOTextures.skull_textures) ){
          auto skull_file = skull_root / names[i];
          if (not names[i].contains("normal") ) {
               tex.create(skull_file.generic_string(), colorSampler,VK_FORMAT_R8G8B8A8_SRGB );
          }else
               tex.create(skull_file.generic_string(), colorSampler,VK_FORMAT_R8G8B8A8_UNORM );
     }

}


void defer::prepareAttachments() {
     setRequiredObjects(mrtFrameBuf.position, mrtFrameBuf.normal, mrtFrameBuf.albedo, mrtFrameBuf.roughness);
     setRequiredObjects(mrtFrameBuf.displace, mrtFrameBuf.depth);
     auto attachmentWidth = simpleSwapchain.swapChainExtent.width;
     auto attachmentHeight = simpleSwapchain.swapChainExtent.height;
     VkImageUsageFlagBits colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
     mrtFrameBuf.position.create(attachmentWidth,attachmentHeight, VK_FORMAT_R16G16B16A16_SFLOAT, colorSampler, colorUsage);
     mrtFrameBuf.normal.create(attachmentWidth,attachmentHeight, VK_FORMAT_R16G16B16A16_SFLOAT, colorSampler, colorUsage);
     mrtFrameBuf.albedo.create(attachmentWidth,attachmentHeight, VK_FORMAT_R8G8B8A8_UNORM, colorSampler, colorUsage);
     mrtFrameBuf.roughness.create(attachmentWidth,attachmentHeight, VK_FORMAT_R8G8B8A8_UNORM, colorSampler, colorUsage);
     mrtFrameBuf.displace.create(attachmentWidth,attachmentHeight, VK_FORMAT_R8G8B8A8_UNORM, colorSampler, colorUsage);
     mrtFrameBuf.depth.create(attachmentWidth,attachmentHeight, FnImage::findDepthFormat(mainDevice.physicalDevice), colorSampler, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void defer::prepareMrtFramebuffer() {
     std::array<VkImageView,6> attachments{};
     attachments[0] = mrtFrameBuf.position.view;
     attachments[1] = mrtFrameBuf.normal.view;
     attachments[2] = mrtFrameBuf.albedo.view;
     attachments[3] = mrtFrameBuf.roughness.view;
     attachments[4] = mrtFrameBuf.displace.view;
     attachments[5] = mrtFrameBuf.depth.view;
     std::cout << "prepareMrtFramebuffer: "<<  mrtFrameBuf.position.view<< std::endl;
     VkFramebufferCreateInfo framebufferCreateInfo{};
     framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
     framebufferCreateInfo.width = simpleSwapchain.swapChainExtent.width;
     framebufferCreateInfo.height = simpleSwapchain.swapChainExtent.height;
     framebufferCreateInfo.renderPass = mrtFrameBuf.renderPass;
     framebufferCreateInfo.attachmentCount = attachments.size();
     framebufferCreateInfo.pAttachments = attachments.data();
     framebufferCreateInfo.layers = 1;
     if(vkCreateFramebuffer(mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &mrtFrameBuf.frameBuffer) != VK_SUCCESS) {
          throw std::runtime_error("failed to create framebuffer");
     }
}
void defer::prepareMrtRenderPass() {
     std::array<VkAttachmentDescription,6> attachmentDescs = {};
     for(auto & attachment : attachmentDescs) {
          attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // before rendering
          attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // after rendering
          attachment.samples = VK_SAMPLE_COUNT_1_BIT;
          attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // 渲染之前stencil 干什么
          attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 渲染之后stencil 干什么
          attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
     }
     attachmentDescs[attachmentDescs.size()-1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;     // set depth
     // format

     attachmentDescs[0].format = mrtFrameBuf.position.format;
     attachmentDescs[1].format = mrtFrameBuf.normal.format;
     attachmentDescs[2].format = mrtFrameBuf.albedo.format;
     attachmentDescs[3].format = mrtFrameBuf.roughness.format;
     attachmentDescs[4].format = mrtFrameBuf.displace.format;
     attachmentDescs[5].format = mrtFrameBuf.depth.format;
     std::cout <<"-----------attachmentDescs[0]." <<magic_enum::enum_name(mrtFrameBuf.position.format) << std::endl;
     // color ref & depth ref
     constexpr std::array<VkAttachmentReference, 5> colorReferences = {{
          {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
          {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
          {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
          {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
          {4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
     }};
     constexpr VkAttachmentReference depthReference = {5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

     VkSubpassDescription subpass{};
     subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
     subpass.colorAttachmentCount = colorReferences.size();
     subpass.pColorAttachments = colorReferences.data();
     subpass.pDepthStencilAttachment = &depthReference;
     // dependency transition
     /*
     VkSubpassDependency dependency_0{};
     dependency_0.srcSubpass = VK_SUBPASS_EXTERNAL;
     dependency_0.dstSubpass = 0;

     dependency_0.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;     // before
     dependency_0.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;    // after

     dependency_0.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;// before
     dependency_0.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;     // after

     VkSubpassDependency dependency_1{};
     dependency_1.srcSubpass = 0;
     dependency_1.dstSubpass = VK_SUBPASS_EXTERNAL;

     dependency_1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
     dependency_1.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

     dependency_1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
     dependency_1.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

     const std::array <VkSubpassDependency, 2> dependencies = {dependency_0, dependency_1};
*/
     // Use subpass dependencies for attachment layout transitions
     std::array<VkSubpassDependency, 2> dependencies;

     dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
     dependencies[0].dstSubpass = 0;
     dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
     dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
     dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
     dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
     dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

     dependencies[1].srcSubpass = 0;
     dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
     dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
     dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
     dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
     dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
     dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


     VkRenderPassCreateInfo renderPassInfo{};
     renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
     renderPassInfo.attachmentCount = attachmentDescs.size();
     renderPassInfo.pAttachments = attachmentDescs.data();
     renderPassInfo.subpassCount = 1;
     renderPassInfo.pSubpasses = &subpass;
     renderPassInfo.dependencyCount = 2;
     renderPassInfo.pDependencies = dependencies.data();

     const auto device = mainDevice.logicalDevice;
     if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &mrtFrameBuf.renderPass)!=VK_SUCCESS) {
          throw std::runtime_error("failed to create render pass!");
     }
}

void defer::prepareDescriptorSets() {
     const auto &device = mainDevice.logicalDevice;
     const auto &physicalDevice = mainDevice.physicalDevice;
     constexpr auto geometryMRT_UBOCount = 1;
     constexpr auto composition_UBOCount = 1;
     constexpr auto uboCount = geometryMRT_UBOCount + composition_UBOCount;

     constexpr auto geometryTexCount = 4;
     constexpr auto compositionTexCount = 5;
     constexpr auto combinedImageSamplerCount = geometryTexCount + compositionTexCount;
     std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboCount},
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, combinedImageSamplerCount}
     }};
     // per object need two set. we have three object.
     // set=0 for ubo, set=1 for texture
     VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 3 * 2); //
     createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
     auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
     assert(result == VK_SUCCESS);

     createGeoDescriptorSets();
     createCompositionDescriptorSets();



}
void defer::createGeoDescriptorSets() {
     constexpr auto combinedImageLayoutBinding = [](const int &binding){return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,binding, VK_SHADER_STAGE_FRAGMENT_BIT);};
     const auto device = mainDevice.logicalDevice;
     std::cout << "create geometry desc sets\n";
     auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);
     const std::array set0_bindings = {set0_ubo_binding0}; // only binding = 0
     const VkDescriptorSetLayoutCreateInfo ubo_createInfo = FnDescriptor::setLayoutCreateInfo(set0_bindings);
     if(vkCreateDescriptorSetLayout(device, &ubo_createInfo, nullptr, &geoDescriptorSets.setLayout0)!=VK_SUCCESS)
          throw std::runtime_error{"Error create plant ubo set layout"};
     // create set1 bindings.
     auto set1_tex_bindings = std::views::iota(0,input_tex_count) | std::views::transform(combinedImageLayoutBinding);
     const auto set1_bindings=  std::ranges::to<std::vector<VkDescriptorSetLayoutBinding>>(set1_tex_bindings);
     const VkDescriptorSetLayoutCreateInfo tex_createInfo = FnDescriptor::setLayoutCreateInfo(set1_bindings);
     auto result = vkCreateDescriptorSetLayout(device, &tex_createInfo, nullptr, &geoDescriptorSets.setLayout1);
     assert(result == VK_SUCCESS);


     const std::array mrtLayouts{geoDescriptorSets.setLayout0, geoDescriptorSets.setLayout1};
     const auto mrtSetAllocateInfo = FnDescriptor::setAllocateInfo(descPool, mrtLayouts);
     if(vkAllocateDescriptorSets(device, &mrtSetAllocateInfo,geoDescriptorSets.ground) != VK_SUCCESS)
          throw std::runtime_error{"can not create ground descriptor set"};
     if(vkAllocateDescriptorSets(device, &mrtSetAllocateInfo,geoDescriptorSets.skull) != VK_SUCCESS)
          throw std::runtime_error{"can not create skull descriptor set"};


     auto updateWriteSets= [this](const VkDescriptorSet set0, const VkDescriptorSet &set1, const Concept::is_range auto & textures) {
          std::vector<VkWriteDescriptorSet> writeSets;
          writeSets.emplace_back(  FnDescriptor::writeDescriptorSet(set0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.mrt.descBufferInfo));
          for(auto &&[k, tex]: UT_Fn::enumerate(textures)) {
               auto writeSet = FnDescriptor::writeDescriptorSet(set1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k , &tex.descImageInfo );
               writeSets.emplace_back(writeSet);
          }
          vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
     };

     updateWriteSets(geoDescriptorSets.ground[0], geoDescriptorSets.ground[1], UBOTextures.ground_textures);
     updateWriteSets(geoDescriptorSets.skull[0],  geoDescriptorSets.skull[1],  UBOTextures.skull_textures);


}

void defer::createCompositionDescriptorSets() {
     const auto device = mainDevice.logicalDevice;
     // composition desc sets
     constexpr auto combinedImageLayoutBinding = [](const int &binding){return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,binding, VK_SHADER_STAGE_FRAGMENT_BIT);};
     std::cout << "create composition desc sets\n";
     auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_FRAGMENT_BIT);
     const std::array set0_bindings = {set0_ubo_binding0}; // only binding = 0
     const VkDescriptorSetLayoutCreateInfo ubo_createInfo = FnDescriptor::setLayoutCreateInfo(set0_bindings);
     // auto result = vkCreateDescriptorSetLayout(device, &tex_createInfo, nullptr, &geoDescriptorSets.setLayout1);
     UT_Fn::invoke_and_check("composition set=0 layout failed", vkCreateDescriptorSetLayout, device, &ubo_createInfo, nullptr, &compositionDescriptorSets.setLayout0 );

     // create set1 bindings. create set layout
     auto set1_tex_bindings = std::views::iota(0,composition_tex_count) | std::views::transform(combinedImageLayoutBinding);
     const auto set1_bindings=  std::ranges::to<std::vector<VkDescriptorSetLayoutBinding>>(set1_tex_bindings);
     const VkDescriptorSetLayoutCreateInfo tex_createInfo = FnDescriptor::setLayoutCreateInfo(set1_bindings);
     UT_Fn::invoke_and_check("composition set=1 failed", vkCreateDescriptorSetLayout,device, &tex_createInfo,nullptr, &compositionDescriptorSets.setLayout1);

     // create sets
     const std::array layouts{compositionDescriptorSets.setLayout0, compositionDescriptorSets.setLayout1};
     const auto allocInfo = FnDescriptor::setAllocateInfo(descPool, layouts);
     UT_Fn::invoke_and_check("Error create comp sets",vkAllocateDescriptorSets,device, &allocInfo,compositionDescriptorSets.composition );

     std::vector<VkWriteDescriptorSet> writeSets;
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.composition.descBufferInfo)); // set = 0, binding =0 ubo
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &mrtFrameBuf.position.descImageInfo));
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &mrtFrameBuf.normal.descImageInfo));
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &mrtFrameBuf.albedo.descImageInfo));
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &mrtFrameBuf.roughness.descImageInfo));
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &mrtFrameBuf.displace.descImageInfo));
     vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}




void defer::preparePipelines() {
     auto device = mainDevice.logicalDevice;
     const auto mrtVertMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/mrt_vert.spv",  device);
     const auto mrtFragMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/mrt_frag.spv",  device);
     const auto deferredVertMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/deferred_vert.spv",  device);
     const auto deferredFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/deferred_frag.spv",  device);

     VkPipelineShaderStageCreateInfo mrtVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, mrtVertMoudule);
     VkPipelineShaderStageCreateInfo mrtFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, mrtFragMoudule);
     VkPipelineShaderStageCreateInfo deferredVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, deferredVertMoudule);
     VkPipelineShaderStageCreateInfo deferredFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, deferredFragModule);
     VkPipelineShaderStageCreateInfo mrtShaderStates[] = {mrtVertShaderStageCreateInfo, mrtFragShaderStageCreateInfo};
     VkPipelineShaderStageCreateInfo deferredShaderStates[] = {deferredVertShaderStageCreateInfo, deferredFragShaderStageCreateInfo};
     // 2. vertex input
     std::array bindings = {GLTFVertex::bindings()};
     auto attribs = GLTFVertex::attribs();
     VkPipelineVertexInputStateCreateInfo vertexInput_ST_CIO = FnPipeline::vertexInputStateCreateInfo(bindings, attribs);

     // 3. assembly
     VkPipelineInputAssemblyStateCreateInfo inputAssembly_ST_CIO = FnPipeline::inputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, VK_FALSE);
     // 4 viewport and scissor
     VkPipelineViewportStateCreateInfo viewport_ST_CIO = FnPipeline::viewPortStateCreateInfo();
     // 5. dynamic state
     auto dynamicsStates = FnPipeline::simpleDynamicsStates();
     VkPipelineDynamicStateCreateInfo dynamics_ST_CIO= FnPipeline::dynamicStateCreateInfo(dynamicsStates);
     // 6. rasterization
     VkPipelineRasterizationStateCreateInfo rasterization_ST_CIO = FnPipeline::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
     // 7. multisampling
     VkPipelineMultisampleStateCreateInfo multisample_ST_CIO=FnPipeline::multisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
     // 8. blending

     std::array<VkPipelineColorBlendAttachmentState, 5> mrtBlendAttachmentStates = {
          FnPipeline::simpleOpaqueColorBlendAttacmentState(), // position;
          FnPipeline::simpleOpaqueColorBlendAttacmentState(), // normal;
          FnPipeline::simpleOpaqueColorBlendAttacmentState(), // albedo
          FnPipeline::simpleOpaqueColorBlendAttacmentState(), // roughness
          FnPipeline::simpleOpaqueColorBlendAttacmentState()  // displace
     };
     VkPipelineColorBlendStateCreateInfo blend_mrt_ST_CIO = FnPipeline::colorBlendStateCreateInfo(mrtBlendAttachmentStates);

     std::array deferredColorBlendAttamentState = {FnPipeline::simpleOpaqueColorBlendAttacmentState()};
     VkPipelineColorBlendStateCreateInfo blend_deferred_ST_CIO = FnPipeline::colorBlendStateCreateInfo(deferredColorBlendAttamentState);

     // 9. pipeline layout
     // 9-1 mrt pipeline layout
     const std::array mrtLayouts{geoDescriptorSets.setLayout0, geoDescriptorSets.setLayout1};
     VkPipelineLayoutCreateInfo mrtLayout_CIO = FnPipeline::layoutCreateInfo(mrtLayouts);
     UT_Fn::invoke_and_check("ERROR create mrt pipeline layout",vkCreatePipelineLayout,device,&mrtLayout_CIO,nullptr, &pipelines.mrtLayout );

     // 9-2 deferred pipeline layout
     const std::array deferredLayouts{compositionDescriptorSets.setLayout0, compositionDescriptorSets.setLayout1};
     VkPipelineLayoutCreateInfo deferredLayout_CIO = FnPipeline::layoutCreateInfo(deferredLayouts);
     UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &deferredLayout_CIO,nullptr, &pipelines.compositionLayout );

     // 10
     VkPipelineDepthStencilStateCreateInfo ds_ST_CIO = FnPipeline::depthStencilStateCreateInfoEnabled();

     // 11. PIPELINE
     VkGraphicsPipelineCreateInfo pipeline_CIO = FnPipeline::pipelineCreateInfo();
     pipeline_CIO.stageCount = 2;

     pipeline_CIO.pVertexInputState = &vertexInput_ST_CIO;
     pipeline_CIO.pInputAssemblyState = &inputAssembly_ST_CIO;
     pipeline_CIO.pViewportState = &viewport_ST_CIO;
     pipeline_CIO.pDynamicState = &dynamics_ST_CIO;
     pipeline_CIO.pMultisampleState = &multisample_ST_CIO;
     pipeline_CIO.pDepthStencilState = &ds_ST_CIO;
     pipeline_CIO.subpass = 0; // ONLY USE ONE PASS

     // 11-1 create mrt pipeline
     pipeline_CIO.pStages = mrtShaderStates;
     pipeline_CIO.pColorBlendState = &blend_mrt_ST_CIO;
     pipeline_CIO.layout = pipelines.mrtLayout;
     pipeline_CIO.renderPass = mrtFrameBuf.renderPass;
     rasterization_ST_CIO.cullMode = VK_CULL_MODE_BACK_BIT;
     pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
     UT_Fn::invoke_and_check( "error create mrt pipeline" ,vkCreateGraphicsPipelines, device, simplePipelineCache.pipelineCache,
          1, &pipeline_CIO, nullptr, &pipelines.mrt);
     // 11-2 create deferred pipeline
     pipeline_CIO.pStages = deferredShaderStates;
     pipeline_CIO.pColorBlendState = &blend_deferred_ST_CIO;
     pipeline_CIO.layout = pipelines.compositionLayout;
     auto emptyVertexInput_ST_CIO =  FnPipeline::vertexInputStateCreateInfo();
     pipeline_CIO.pVertexInputState = &emptyVertexInput_ST_CIO;
     rasterization_ST_CIO.cullMode = VK_CULL_MODE_FRONT_BIT;
     pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
     pipeline_CIO.renderPass = simplePass.pass; // use our main render pass
     UT_Fn::invoke_and_check( "error create deferred pipeline" ,vkCreateGraphicsPipelines, device, simplePipelineCache.pipelineCache,
         1, &pipeline_CIO, nullptr, &pipelines.composition);

     UT_Fn::cleanup_shader_module(device, mrtVertMoudule, mrtFragMoudule);
     UT_Fn::cleanup_shader_module(device, deferredVertMoudule, deferredFragModule);
}


void defer::prepareUniformBuffers() {
     setRequiredObjects(uniformBuffers.mrt, uniformBuffers.composition);
     uniformBuffers.mrt.createAndMapping(sizeof(mrtData));
     uniformBuffers.composition.createAndMapping(sizeof(compositionData));
     // Setup instanced model positions
     mrtData.instancePos[0] = glm::vec4(0);
     mrtData.instancePos[1] = glm::vec4(128.0f, 8.0, 13, 0.0f);
     mrtData.instancePos[2] = glm::vec4(23.7558, 8.08336, -52.067f, 0.0f);

     mrtData.instanceRot[0] = glm::vec4(0,0,0,0);
     mrtData.instanceRot[1] = glm::vec4(0,35.955,0,0);
     mrtData.instanceRot[2] = glm::vec4(0,-40,0,0);

     mrtData.instanceScale[0] = glm::vec4(1.0);
     mrtData.instanceScale[1] = glm::vec4(3.0);
     mrtData.instanceScale[2] = glm::vec4(2.0);

     updateUniformBuffers();
}
void defer::updateUniformBuffers() {
     auto [width, height] = simpleSwapchain.swapChainExtent;
     mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
     mrtData.model = glm::mat4(1.0f);
     mrtData.projection = mainCamera.projection();
     mrtData.projection[1][1] *= -1;
     mrtData.view = mainCamera.view();
     memcpy(uniformBuffers.mrt.mapped, &mrtData, sizeof(mrtData));

     // Current view position
     compositionData.viewPos = glm::vec4(mainCamera.mPosition, 0.0f);
     compositionData.lights[0] = {
          {482.972,231.21,124.495 ,0 },{10,10,10},800
     };
     compositionData.lights[1] = {
          {-204.601,98.3806,0 ,0 },{0.533 * 5,0.647415*5,1*5},600
     };
     memcpy(uniformBuffers.composition.mapped, &compositionData, sizeof(compositionData));
}


void defer::render() {

     updateUniformBuffers();
     recordMrtCommandBuffer();
     recordCompositionCommandBuffer();

     //
     //    [[WAIT]]              ->         [[SIGNAL]]
     // imageAvailableSemaphore  ->      mrt semaphore
     // mrt semaphore            ->      renderFinishedSemaphore
     // renderFinished           ->      present
     //

     VkSubmitInfo mrtSubmitInfo = {};
     mrtSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
     mrtSubmitInfo.commandBufferCount = 1;
     mrtSubmitInfo.waitSemaphoreCount = 1;
     mrtSubmitInfo.signalSemaphoreCount = 1;
     mrtSubmitInfo.pWaitDstStageMask = waitStages;
     mrtSubmitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFlightFrame];     // Wait for swap chain presentation to finish
     mrtSubmitInfo.pSignalSemaphores = & mrtSemaphores[currentFlightFrame];     // Signal ready with offscreen semaphore
     // Submit mrt work
     mrtSubmitInfo.pCommandBuffers = &mrtCommandBuffers[currentFlightFrame];
     UT_Fn::invoke_and_check("error submit mrt queue", vkQueueSubmit, mainDevice.graphicsQueue, 1, &mrtSubmitInfo, VK_NULL_HANDLE);

     VkSubmitInfo compSubmitInfo = {};
     compSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
     compSubmitInfo.commandBufferCount = 1;
     compSubmitInfo.waitSemaphoreCount = 1;
     compSubmitInfo.signalSemaphoreCount = 1;
     compSubmitInfo.pWaitDstStageMask = waitStages;
     // Wait for offscreen semaphore
     compSubmitInfo.pWaitSemaphores = &mrtSemaphores[currentFlightFrame];
     // Signal ready with render complete semaphore
     compSubmitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFlightFrame];
     // Submit composition work
     compSubmitInfo.pCommandBuffers = &activatedFrameCommandBufferToSubmit;
     UT_Fn::invoke_and_check("error submit render composition queue",vkQueueSubmit, mainDevice.graphicsQueue, 1, &compSubmitInfo, inFlightFences[currentFlightFrame]);

     presentMainCommandBufferFrame();



}

void defer::createMrtCommandBuffers() {
     VkCommandBufferAllocateInfo cbAllocInfo = {};
     cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
     cbAllocInfo.commandPool = graphicsCommandPool ;
     cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
     cbAllocInfo.commandBufferCount = 2;
     UT_Fn::invoke_and_check("create mrt commandBuffers error", vkAllocateCommandBuffers,
          mainDevice.logicalDevice, &cbAllocInfo, mrtCommandBuffers);
     VkSemaphoreCreateInfo semaphore_CIO{};
     semaphore_CIO.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
     for(auto & mrtSemaphore : mrtSemaphores) {
          UT_Fn::invoke_and_check("create mrt semaphores error",vkCreateSemaphore, mainDevice.logicalDevice, &semaphore_CIO, nullptr, &mrtSemaphore);
     }
}


void defer::swapChainResize() {
     cleanupMrtFramebuffer();
     prepareAttachments();

     std::vector<VkWriteDescriptorSet> writeSets;
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.composition.descBufferInfo)); // set = 0, binding =0 ubo
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &mrtFrameBuf.position.descImageInfo));
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &mrtFrameBuf.normal.descImageInfo));
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &mrtFrameBuf.albedo.descImageInfo));
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &mrtFrameBuf.roughness.descImageInfo));
     writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &mrtFrameBuf.displace.descImageInfo));
     vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
     prepareMrtFramebuffer();
}
void defer::cleanupMrtFramebuffer() {
     mrtFrameBuf.position.cleanup();
     mrtFrameBuf.normal.cleanup();
     mrtFrameBuf.albedo.cleanup();
     mrtFrameBuf.roughness.cleanup();
     mrtFrameBuf.displace.cleanup();
     mrtFrameBuf.depth.cleanup();
     vkDestroyFramebuffer(mainDevice.logicalDevice, mrtFrameBuf.frameBuffer, nullptr);
}
void defer::recordMrtCommandBuffer() {

     // Clear values for all attachments written in the fragment shader
     std::vector<VkClearValue> clearValues{};
     clearValues.resize(6);
     // position, normal, albedo, roughness, displace;
     clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
     clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
     clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
     clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
     clearValues[4].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
     clearValues[5].depthStencil = { 1.0f, 0 };

     auto mrtCommandBuffer = mrtCommandBuffers[currentFlightFrame];
     vkResetCommandBuffer(mrtCommandBuffer,/*VkCommandBufferResetFlagBits*/ 0); //0: command buffer reset

     const VkFramebuffer &framebuffer = mrtFrameBuf.frameBuffer;
     auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(framebuffer,
          mrtFrameBuf.renderPass,
         &simpleSwapchain.swapChainExtent,clearValues);

     auto result = vkBeginCommandBuffer(mrtCommandBuffer, &cmdBufferBeginInfo);
     if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
     vkCmdBeginRenderPass(mrtCommandBuffer, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
     vkCmdBindPipeline(mrtCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , pipelines.mrt);

     VkDeviceSize offsets[1] = { 0 };
     auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
     auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
     vkCmdSetViewport(mrtCommandBuffer, 0, 1, &viewport);
     vkCmdSetScissor(mrtCommandBuffer,0, 1, &scissor);
     vkCmdBindVertexBuffers(mrtCommandBuffer, 0, 1, &ground_gltf.parts[0].verticesBuffer, offsets);
     vkCmdBindIndexBuffer(mrtCommandBuffer,ground_gltf.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
     vkCmdBindDescriptorSets(mrtCommandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.mrtLayout, 0, 2, geoDescriptorSets.ground, 0, nullptr);
     vkCmdDrawIndexed(mrtCommandBuffer, ground_gltf.parts[0].indices.size(), 1, 0, 0, 0);

     vkCmdBindVertexBuffers(mrtCommandBuffer, 0, 1, &skull_gltf.parts[0].verticesBuffer, offsets);
     vkCmdBindIndexBuffer(mrtCommandBuffer,skull_gltf.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
     vkCmdBindDescriptorSets(mrtCommandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.mrtLayout, 0, 2, geoDescriptorSets.skull, 0, nullptr);
     vkCmdDrawIndexed(mrtCommandBuffer, skull_gltf.parts[0].indices.size(), 3, 0, 0, 0);
     vkCmdEndRenderPass(mrtCommandBuffer);
     UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,mrtCommandBuffer );
}

void defer::recordCompositionCommandBuffer() {
     std::vector<VkClearValue> clearValues(2);
     clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
     clearValues[1].depthStencil = {1.0f, 0};
     const VkFramebuffer &framebuffer = activatedSwapChainFramebuffer;
     auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(framebuffer,
         simplePass.pass,
         &simpleSwapchain.swapChainExtent,clearValues);
     auto result = vkBeginCommandBuffer(activatedFrameCommandBufferToSubmit, &cmdBufferBeginInfo);
     if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
     vkCmdBeginRenderPass(activatedFrameCommandBufferToSubmit, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
     vkCmdBindPipeline(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS ,pipelines.composition);
     auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
     auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
     vkCmdSetViewport(activatedFrameCommandBufferToSubmit, 0, 1, &viewport);
     vkCmdSetScissor(activatedFrameCommandBufferToSubmit,0, 1, &scissor);

     vkCmdBindDescriptorSets(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.compositionLayout,
          0, 2, compositionDescriptorSets.composition, 0, nullptr);
     vkCmdBindPipeline(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
     vkCmdDraw(activatedFrameCommandBufferToSubmit, 3, 1, 0, 0);
     vkCmdEndRenderPass(activatedFrameCommandBufferToSubmit);
     UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,activatedFrameCommandBufferToSubmit );
}


LLVK_NAMESPACE_END