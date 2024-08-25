//
// Created by liuya on 8/16/2024.
//

#include "deferred.h"
#include <ranges>
#include <algorithm>
#include <filesystem>

#include  "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
LLVK_NAMESPACE_BEGIN
defer::defer() {
     mainCamera.mPosition = {0,10,0};
}


void defer::cleanupObjects() {
     uniformBuffers.composition.cleanup();
     uniformBuffers.mrt.cleanup();
     vkDestroyRenderPass(mainDevice.logicalDevice, mrtFrameBuf.renderPass, nullptr);
     vkDestroyFramebuffer(mainDevice.logicalDevice, mrtFrameBuf.frameBuffer, nullptr);
     vkDestroySampler(mainDevice.logicalDevice,colorSampler, nullptr);
     vkDestroyDescriptorPool(mainDevice.logicalDevice, descPool, nullptr);
     UT_Fn::cleanup_descriptor_set_layout(mainDevice.logicalDevice, geoDescriptorSets.setLayout0, geoDescriptorSets.setLayout1);
     UT_Fn::cleanup_descriptor_set_layout(mainDevice.logicalDevice, compositionDescriptorSets.setLayout0, compositionDescriptorSets.setLayout1);
     UT_Fn::cleanup_pipeline_layout(mainDevice.logicalDevice,pipelines.compositionLayout, pipelines.mrtLayout);
     for(auto &tex: UBOTextures.ground_textures) {
          tex.cleanup();
     }
     for(auto &tex: UBOTextures.skull_textures) {
          tex.cleanup();
     }
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
     fs::path root = "content/deferred";
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
     if(vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool)!=VK_SUCCESS)
          throw std::runtime_error{"ERROR create descriptor pool"};

     constexpr auto combinedImageLayoutBinding = [](const int &binding){return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,binding, VK_SHADER_STAGE_FRAGMENT_BIT);};
     // for ground and skull geometry
     {
          //create set0 bindings
          std::cout << "create geometry desc sets\n";
          auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);
          const std::array set0_bindings = {set0_ubo_binding0}; // only binding = 0
          const VkDescriptorSetLayoutCreateInfo ubo_createInfo = FnDescriptor::setLayoutCreateInfo(set0_bindings);
          if(vkCreateDescriptorSetLayout(device, &ubo_createInfo, nullptr, &geoDescriptorSets.setLayout0)!=VK_SUCCESS)
               throw std::runtime_error{"Error create plant ubo set layout"};
          // create set1 bindings.
          auto set1_tex_bindings = std::views::iota(0,UBOTextures.tex_count) | std::views::transform(combinedImageLayoutBinding);
          const auto set1_bindings=  std::ranges::to<std::vector<VkDescriptorSetLayoutBinding>>(set1_tex_bindings);
          const VkDescriptorSetLayoutCreateInfo tex_createInfo = FnDescriptor::setLayoutCreateInfo(set1_bindings);
          if(vkCreateDescriptorSetLayout(device, &tex_createInfo, nullptr, &geoDescriptorSets.setLayout1) != VK_SUCCESS) {
               throw std::runtime_error{"Error create plant tex set layout"};
          }

          const std::array mrtLayouts{geoDescriptorSets.setLayout0, geoDescriptorSets.setLayout1};
          const auto mrtSetAllocateInfo = FnDescriptor::setAllocateInfo(descPool, mrtLayouts);
          if(vkAllocateDescriptorSets(device, &mrtSetAllocateInfo,geoDescriptorSets.ground) != VK_SUCCESS)
               throw std::runtime_error{"can not create ground descriptor set"};
          if(vkAllocateDescriptorSets(device, &mrtSetAllocateInfo,geoDescriptorSets.skull) != VK_SUCCESS)
               throw std::runtime_error{"can not create skull descriptor set"};
     }
     // composition desc sets
     {
          std::cout << "create composition desc sets\n";
          auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_FRAGMENT_BIT);
          const std::array set0_bindings = {set0_ubo_binding0}; // only binding = 0
          const VkDescriptorSetLayoutCreateInfo ubo_createInfo = FnDescriptor::setLayoutCreateInfo(set0_bindings);
          if(vkCreateDescriptorSetLayout(device, &ubo_createInfo, nullptr, &compositionDescriptorSets.setLayout0)!=VK_SUCCESS)
               throw std::runtime_error{"Error create plant ubo set layout"};

          // create set1 bindings.
          auto set1_tex_bindings = std::views::iota(0,compositionDescriptorSets.tex_count) | std::views::transform(combinedImageLayoutBinding);
          const auto set1_bindings=  std::ranges::to<std::vector<VkDescriptorSetLayoutBinding>>(set1_tex_bindings);
          const VkDescriptorSetLayoutCreateInfo tex_createInfo = FnDescriptor::setLayoutCreateInfo(set1_bindings);
          if(vkCreateDescriptorSetLayout(device, &tex_createInfo, nullptr, &compositionDescriptorSets.setLayout1)!=VK_SUCCESS)
               throw std::runtime_error{"Error create plant tex set layout"};
     }




}
void defer::preparePipelines() {

}


void defer::prepareUniformBuffers() {
     setRequiredObjects(uniformBuffers.mrt, uniformBuffers.composition);
     uniformBuffers.mrt.createAndMapping(sizeof(mrtData));
     uniformBuffers.composition.createAndMapping(sizeof(compositionData));
}
void defer::updateUniformBuffers() {

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