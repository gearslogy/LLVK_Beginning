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
#include "Pipeline.hpp"
LLVK_NAMESPACE_BEGIN
defer::defer() {
     mainCamera.mPosition = {154.865,205.119,198.508};
     auto [width, height] = simpleSwapchain.swapChainExtent;
     mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
}


void defer::cleanupObjects() {
     auto device = mainDevice.logicalDevice;
     uniformBuffers.composition.cleanup();
     uniformBuffers.mrt.cleanup();
     vkDestroyRenderPass(device, mrtFrameBuf.renderPass, nullptr);
     vkDestroyFramebuffer(device, mrtFrameBuf.frameBuffer, nullptr);
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
     ground_gltf.load("content/ground/ground.gltf");
     skull_gltf.load("content/skull/skull.gltf");
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
     auto skull_root = root / "albedo";

     // read ground textures
     for(auto &&[i, tex] : UT_Fn::enumerate(UBOTextures.ground_textures) ){
          auto ground_file = ground_root / names[i];
          tex.create(ground_file.generic_string(), colorSampler );
     }
     // read skull textures
     for(auto &&[i, tex] : UT_Fn::enumerate(UBOTextures.skull_textures) ){
          auto skull_file = skull_root / names[i];
          tex.create(skull_file.generic_string(), colorSampler );
     }

}



void defer::createAttachment(const VkFormat &format, const VkImageUsageFlagBits &usage, IVmaUBOTexture & attachment ) {
     const auto device = mainDevice.logicalDevice;
     const auto physicalDevice = mainDevice.physicalDevice;
     const auto width = simpleSwapchain.swapChainExtent.width;
     const auto height = simpleSwapchain.swapChainExtent.height;
     // 1. build req objects. we use vma allocation
     VmaBufferRequiredObjects req{};
     req.allocator = vmaAllocator;
     req.device = device;
     req.physicalDevice = physicalDevice;
     req.queue = mainDevice.graphicsQueue;
     req.commandPool = graphicsCommandPool;
     // 2. create image and allocation
     FnVmaImage::createImageAndAllocation(req, width, height, 1, 1,
      format,
      VK_IMAGE_TILING_OPTIMAL, usage | VK_IMAGE_USAGE_SAMPLED_BIT,
      false,
      attachment.image,
      attachment.imageAllocation);
     // 3. assign to format
     attachment.format = format;
     // 4. make sure the aspect mask type
     VkImageAspectFlags aspectMask = 0;
     if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
          aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
     }
     if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
          aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
          if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
               aspectMask |=VK_IMAGE_ASPECT_STENCIL_BIT;
     }
     // 5. create image view
     FnImage::createImageView(device, attachment.image, format,1,1,
          aspectMask, attachment.view);
}

void defer::prepareMrtFramebuffer() {
     createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, mrtFrameBuf.position);
     createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, mrtFrameBuf.normal);
     createAttachment(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, mrtFrameBuf.albedo);
     createAttachment(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, mrtFrameBuf.roughness);
     createAttachment(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, mrtFrameBuf.displace);
     createAttachment(FnImage::findDepthFormat(mainDevice.physicalDevice),VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,mrtFrameBuf.depth); // depth

     std::array<VkImageView,6> attachments{};
     attachments[0] = mrtFrameBuf.position.view;
     attachments[1] = mrtFrameBuf.normal.view;
     attachments[2] = mrtFrameBuf.albedo.view;
     attachments[3] = mrtFrameBuf.roughness.view;
     attachments[4] = mrtFrameBuf.displace.view;
     attachments[5] = mrtFrameBuf.depth.view;

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

     dependency_1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
     dependency_1.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

     dependency_1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
     dependency_1.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

     const std::array <VkSubpassDependency, 2> dependencies = {dependency_0, dependency_1};

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

     // write sets
     std::vector<VkWriteDescriptorSet> groundWriteSets;
     groundWriteSets.emplace_back(  FnDescriptor::writeDescriptorSet(geoDescriptorSets.ground[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.mrt.descBufferInfo));

     for(auto &&[k, tex]: UT_Fn::enumerate(UBOTextures.ground_textures)) {
          auto writeSet = FnDescriptor::writeDescriptorSet(geoDescriptorSets.ground[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k , &uniformBuffers.mrt.descBufferInfo);
          groundWriteSets.emplace_back(writeSet);
     }
     vkUpdateDescriptorSets(device, static_cast<uint32_t>(groundWriteSets.size()), groundWriteSets.data(), 0, nullptr);


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


     std::vector<VkWriteDescriptorSet> skullWriteSets;
     skullWriteSets.emplace_back(  FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.mrt.descBufferInfo)); // set = 0, binding =0 ubo
     // tex write set
     std::array <VkDescriptorImageInfo, composition_tex_count> texImageInfos{};
     for(auto &t : texImageInfos) {
          t.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          t.sampler = colorSampler;
     }
     texImageInfos[0].imageView = mrtFrameBuf.position.view;
     texImageInfos[1].imageView = mrtFrameBuf.normal.view;
     texImageInfos[2].imageView = mrtFrameBuf.albedo.view;
     texImageInfos[3].imageView = mrtFrameBuf.roughness.view;
     texImageInfos[4].imageView = mrtFrameBuf.displace.view;
     for(const auto idx: UT_Fn::xrange(texImageInfos)) {
          skullWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(compositionDescriptorSets.composition[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, idx, &texImageInfos[idx]));
     }
     vkUpdateDescriptorSets(device, static_cast<uint32_t>(skullWriteSets.size()), skullWriteSets.data(), 0, nullptr);
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
     UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device,&deferredLayout_CIO,nullptr, &pipelines.composition );

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
     mrtData.instancePos[0] = glm::vec4(30.0f,11,0,0);
     mrtData.instancePos[1] = glm::vec4(128.0f, 8.0, 13, 0.0f);
     mrtData.instancePos[2] = glm::vec4(23, 8, -52.0f, 0.0f);
     mrtData.instanceRot[0] = glm::vec4(0,-42,0,0);
     mrtData.instanceRot[1] = glm::vec4(0,35,0,0);
     mrtData.instanceRot[2] = glm::vec4(0,-93,0,0);
     mrtData.instanceScale[0] = 2;
     mrtData.instanceScale[1] = 2;
     mrtData.instanceScale[2] = 2;

     updateUniformBuffers();
}
void defer::updateUniformBuffers() {
     auto [width, height] = simpleSwapchain.swapChainExtent;
     mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);

     mrtData.projection = mainCamera.projection();
     mrtData.projection[1][1] *= -1;
     mrtData.view = mainCamera.view();
     memcpy(uniformBuffers.mrt.mapped, &mrtData, sizeof(mrtData));

     // Current view position
     compositionData.viewPos = glm::vec4(mainCamera.mPosition, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
     compositionData.lights[0] = {
          {482.972,231.21,124.495 ,0 },{1,1,1},300
     };
     compositionData.lights[1] = {
          {-204.601,98.3806,0 ,0 },{0.533,0.647415,1},300
     };
     memcpy(uniformBuffers.composition.mapped, &compositionData, sizeof(compositionData));
}


void defer::render() {

}

void defer::swapChainResize() {
     cleanupMrtFramebuffer();
     prepareMrtFramebuffer();
}
void defer::cleanupMrtFramebuffer() {
     auto clean = [this](auto & ... attachment) {
          (vmaDestroyImage(this->vmaAllocator, attachment.image, attachment.imageAllocation) , ... );
          (vkDestroyImageView(this->mainDevice.logicalDevice, attachment.view, nullptr), ... );
     };
     clean( mrtFrameBuf.position, mrtFrameBuf.albedo,
          mrtFrameBuf.normal,
          mrtFrameBuf.roughness,
          mrtFrameBuf.displace,
          mrtFrameBuf.depth );
}
void defer::recordMrtCommandBuffer() {

}

void defer::recordCompositionCommandBuffer() {

}


LLVK_NAMESPACE_END