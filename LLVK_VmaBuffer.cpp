//
// Created by liuya on 8/4/2024.
//

#include "LLVK_vmaBuffer.h"
#include "libs/stb_image.h"
#include "LLVK_Image.h"
#include <ktx.h>
#include "libs/magic_enum.hpp"
LLVK_NAMESPACE_BEGIN
    void VmaUBOBuffer::createAndMapping(VkDeviceSize bufferSize) {
    auto result = FnVmaBuffer::createBuffer(requiredObjects.device,
        requiredObjects.allocator,
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        true,
        bufferAndAllocation.buffer, bufferAndAllocation.allocation);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR create stagging vma buffer"};
    bufferAndAllocation.memorySize = bufferSize;
    descBufferInfo.buffer = bufferAndAllocation.buffer;
    descBufferInfo.offset = 0;
    descBufferInfo.range = bufferSize;
    vmaMapMemory(requiredObjects.allocator, bufferAndAllocation.allocation, &mapped);
}
void VmaUBOBuffer::cleanup() {
    vmaUnmapMemory(requiredObjects.allocator, bufferAndAllocation.allocation);
    FnVmaBuffer::destroyBuffer(requiredObjects.allocator ,bufferAndAllocation.buffer,bufferAndAllocation.allocation );
}


void VmaSimpleGeometryBufferManager::cleanup() {
    for(auto &vma : createVertexBuffers)
        FnVmaBuffer::destroyBuffer(requiredObjects.allocator,vma.buffer, vma.allocation);
    for(auto &vma : createIndexedBuffers)
        FnVmaBuffer::destroyBuffer(requiredObjects.allocator,vma.buffer, vma.allocation);
    for(auto &vma : createIndirectBuffers) {
        FnVmaBuffer::destroyBuffer(requiredObjects.allocator,vma.buffer, vma.allocation);
    }
}

//  image
void FnVmaImage::createImageAndAllocation(const VmaBufferRequiredObjects &reqObj,
                                             uint32_t width, uint32_t height,
                                             uint32_t mipLevels, uint32_t layerCount,
                                             const VkFormat format,
                                             VkImageTiling tiling,
                                             VkImageUsageFlags usageFlags,
                                             bool canMapping,
                                             VkImage &image, VmaAllocation &imageAllocation) {
    const auto & [device,physicalDevice,commandPool, queue, allocator] = reqObj;

    VkImageCreateInfo imageInfo = FnImage::imageCreateInfo(width,height);
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = layerCount;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.usage = usageFlags;
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if(canMapping) allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    if(vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &image, &imageAllocation, nullptr) !=VK_SUCCESS) {
        throw std::runtime_error{"can not create image and allocation"};
    }
}
void FnVmaImage::createImageAndAllocation(const VmaBufferRequiredObjects &reqObj, const VkImageCreateInfo &createInfo,  bool canMapping, VkImage &image, VmaAllocation &allocation) {
    const auto & [device,physicalDevice,commandPool, queue, allocator] = reqObj;
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if(canMapping) allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    if(vmaCreateImage(allocator, &createInfo, &allocCreateInfo, &image, &allocation, nullptr) !=VK_SUCCESS) {
        throw std::runtime_error{"can not create image and allocation"};
    }
}

void VmaAttachment::create(uint32_t width, uint32_t height,
    const VkFormat &attachFormat,
    const VkSampler & sampler,
    const VkImageUsageFlags &usage) {
    // 2. create image and allocation
    format = attachFormat;
    FnVmaImage::createImageAndAllocation(requiredObjects, width, height, 1, 1,
     format,
     VK_IMAGE_TILING_OPTIMAL, usage | VK_IMAGE_USAGE_SAMPLED_BIT,
     false,
     image,
     imageAllocation);
    // 3. make sure the aspect mask type
    VkImageAspectFlags aspectMask = 0;
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT){
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
            aspectMask |=VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    // 4. create image view
    FnImage::createImageView(requiredObjects.device, image, format,aspectMask,1, 1, view);
    descImageInfo.sampler = sampler;
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descImageInfo.imageView = view;
}
void VmaAttachment::cleanup() {
    vmaDestroyImage(requiredObjects.allocator, image, imageAllocation);
    vkDestroyImageView(requiredObjects.device, view, nullptr);
}
void VmaAttachment::createDepth32(uint32_t width, uint32_t height,
        const VkSampler & sampler) {
    format = VK_FORMAT_D32_SFLOAT;// d32只用这个够了。
    //format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    //format = FnImage::findDepthFormat(requiredObjects.physicalDevice);
    VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_DEPTH_BIT; // 这个很关键 imageview要用。如果要stencil，必须还得一个imageview
    FnVmaImage::createImageAndAllocation(requiredObjects, width, height, 1, 1,
                                         format,
                                         VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                         false,
                                         image,
                                         imageAllocation);
    // 4. create image view
    FnImage::createImageView(requiredObjects.device, image, format,aspect,1, 1, view);


    descImageInfo.sampler = sampler;
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // finally need transition to this layout
    descImageInfo.imageView = view;
}

void VmaAttachment::create2dArrayDepth32(uint32_t width, uint32_t height, uint32_t layerCount,
        const VkSampler & sampler) {
    format = VK_FORMAT_D32_SFLOAT;// d32只用这个够了。
    VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_DEPTH_BIT; // 这个很关键 imageview要用。如果要stencil，必须还得一个imageview
    FnVmaImage::createImageAndAllocation(requiredObjects, width, height, 1, layerCount,
                                         format,
                                         VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                         false,
                                         image,
                                         imageAllocation);
    // 4. create image view
    auto imageViewCIO = FnImage::imageViewCreateInfo(image, format);
    imageViewCIO.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    imageViewCIO.subresourceRange.aspectMask = aspect;
    imageViewCIO.subresourceRange.layerCount = layerCount;
    FnImage::createImageView(requiredObjects.device, imageViewCIO, view);
    //FnImage::createImageView(requiredObjects.device, image, format,aspect,1, 1, view);
    descImageInfo.sampler = sampler;
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // finally need transition to this layout
    descImageInfo.imageView = view;
}


void VmaDepthStencilAttachment::create(uint32_t width, uint32_t height, const VkSampler &sampler) {
    FnVmaImage::createImageAndAllocation(requiredObjects, width, height, 1, 1,
     format,
     VK_IMAGE_TILING_OPTIMAL,
     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
     false,
     image,
     imageAllocation);
    // 4. create image view
    VkImageAspectFlagBits depthAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    FnImage::createImageView(requiredObjects.device, image, format,depthAspectMask,1, 1, depthView);
    descDepthImageInfo.sampler = sampler;
    descDepthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descDepthImageInfo.imageView = depthView;

    VkImageAspectFlagBits stencilAspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
    FnImage::createImageView(requiredObjects.device, image, format,stencilAspectMask,1, 1, stencilView);
    descStencilImageInfo.sampler = sampler;
    descStencilImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descStencilImageInfo.imageView = stencilView;
}



void VmaDepthStencilAttachment::cleanup() {
    vmaDestroyImage(requiredObjects.allocator, image, imageAllocation);
    vkDestroyImageView(requiredObjects.device, depthView, nullptr);
    vkDestroyImageView(requiredObjects.device, stencilView, nullptr);
}




void FnVmaImage::createTexture(const VmaBufferRequiredObjects &reqObj,const VkFormat format,
    const std::string &filePath,VkImage &image, VmaAllocation &allocation,
    uint32_t &createdMipLevels) {
    const auto &[device,physicalDevice,commandPool, queue, allocator] = reqObj;
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    // caculate num mip levels
    auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    createdMipLevels = mipLevels;
    // create staging
    VkBuffer stagingBuffer{};
    VmaAllocation stagingBufferAllocation{};
    FnVmaBuffer::createBuffer(device, allocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        true,stagingBuffer, stagingBufferAllocation);
    // mapping stagging
    void* data;
    vmaMapMemory(allocator, stagingBufferAllocation, &data);
    memcpy(data, pixels, imageSize);
    vmaUnmapMemory(allocator, stagingBufferAllocation);
    stbi_image_free(pixels);

    // create our image and allocation

    //constexpr VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    createImageAndAllocation(reqObj, texWidth, texHeight,
        mipLevels,1,
        format,VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        false,
        image, allocation );

    // 3. copy buffer to image
    FnImage::transitionImageLayout(device, commandPool, queue,
                          image, format,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels,1);

    FnImage::copyBufferToImage(device, commandPool, queue, stagingBuffer,
                      image, texHeight, texHeight);

    FnImage::generateMipmaps(physicalDevice, device,
        commandPool, queue,
        image,
        format,
        texWidth,texHeight,
        mipLevels);

    vmaDestroyBuffer(allocator, stagingBuffer,stagingBufferAllocation);
}

void VmaUBOTexture::create(const std::string &file, const VkSampler &sampler,const VkFormat &imageFormat) {
    //constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB; // need gamma correct
    //constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM; // do not need gamma correct
    uint32_t createMipLevels{};
    FnVmaImage::createTexture(requiredObjects, imageFormat, file, image, imageAllocation,createMipLevels);
    FnImage::createImageView( requiredObjects.device, image,
            imageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT,
            createMipLevels,1,view);
    format = imageFormat;
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descImageInfo.imageView = view;
    descImageInfo.sampler = sampler;
}

void VmaUBOTexture::cleanup() {
    vmaDestroyImage(requiredObjects.allocator, image, imageAllocation);
    vkDestroyImageView(requiredObjects.device, view, nullptr);
}

void VmaUBOKTXTexture::cleanup() {
    vmaDestroyImage(requiredObjects.allocator, image, imageAllocation);
    vkDestroyImageView(requiredObjects.device, view, nullptr);
}

void VmaUBOKTXTexture::create(const std::string &file, VkSampler sampler) {
    const auto &[device,physicalDevice,commandPool, queue, allocator] = requiredObjects;
    ktxTexture* ktx;


    KTX_error_code result = ktxTexture_CreateFromNamedFile(file.c_str(),
                                            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                            &ktx);

    if(result!= KTX_SUCCESS)
        throw std::runtime_error{"error loading ktx"};

    uint32_t numLevels = ktx->numLevels;
    uint32_t baseWidth = ktx->baseWidth;
    uint32_t baseHeight = ktx->baseHeight;
    uint32_t numLayers = ktx->numLayers;
    bool isArray = ktx->isArray;

    ktxTexture2* ktx2 = reinterpret_cast<ktxTexture2*>(ktx); // if ktx->classId == ktxTexture2_c
    uint32_t component = ktxTexture2_GetNumComponents(ktx2);
    std::cout << "component:" << component << std::endl;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    std::cout << ktx2->dataSize <<std::endl;

    //std::cout << magic_enum::enum_name(ktx2->supercompressionScheme)<< std::endl;
        //std::cout << magic_enum::enum_name(ktxTexture2_GetColorModel_e(ktx2)) << std::endl; // KHR_DF_MODEL_UASTC


    ktx_size_t tempOffset;
    result = ktxTexture_GetImageOffset(ktx, 0, 0, 0, &tempOffset);
    std::cout << "temp offset:" << tempOffset << std::endl;



    // 0 Get Staging buffer size
    VkDeviceSize imageSize = ktxTexture_GetDataSize(ktx); // all buffer size
    std::cout << "widht:" << ktx->baseWidth << " height:" << ktx->baseHeight << std::endl;
    std::cout << "numLevels:" << numLevels << " numLayers:" << numLayers << "   total bytes:" << imageSize << std::endl;
    // 创建Staging Buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    FnVmaBuffer::createBuffer(device,allocator, imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true,
                 stagingBuffer,
                 stagingAllocation);

    // 复制所有数据到 Staging Buffer，包括每一层level，每一层layer
    void* data;
    vmaMapMemory(allocator, stagingAllocation, &data);
    memcpy(data, ktxTexture_GetData(ktx), imageSize);
    vmaUnmapMemory(allocator, stagingAllocation);



    std::vector<VkBufferImageCopy> bufferCopyRegions;
    for(auto layer : UT_Fn::xrange(0, numLayers)) {
        for(auto mipLevel: UT_Fn::xrange(0,numLevels)) {

            // Retrieve a pointer to the image for a specific mip level, array layer & face or depth slice.
            int faceSlice = 0;
            ktx_size_t offset;
            result = ktxTexture_GetImageOffset(ktx, static_cast<ktx_uint32_t>(mipLevel), static_cast<ktx_uint32_t>(layer), faceSlice, &offset);
            if(result!= KTX_SUCCESS)
                throw std::runtime_error{"error ktxTexture_GetImageOffset ktx"};

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = mipLevel;
            bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = std::max(1u, baseWidth >> mipLevel);
            bufferCopyRegion.imageExtent.height = std::max(1u, baseHeight >> mipLevel);
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;
            bufferCopyRegions.push_back(bufferCopyRegion);
        }
    }


    //2. create DST image & allocation
    auto imageCreateInfo = FnImage::imageCreateInfo(baseWidth, baseHeight);
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    FnVmaImage::createImageAndAllocation(requiredObjects,imageCreateInfo,false, image, imageAllocation);
    // DST imageview
    auto viewCIO = FnImage::imageViewCreateInfo(image, format);
    viewCIO.format = format;
    viewCIO.subresourceRange.levelCount = 1;
    viewCIO.subresourceRange.layerCount = 1;
    FnImage::createImageView(device, viewCIO,view);

    FnImage::transitionImageLayout(device, commandPool, queue,image,
                          format,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,1);

    //3. do staging -> dst image

    auto copyCmd = FnCommand::beginSingleTimeCommand(device, commandPool);
    // Copy mip levels from staging buffer
    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.bufferRowLength = 0;  // 设置为图像的宽度
    bufferCopyRegion.bufferImageHeight = 0;  // 设置为图像的高度
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = 0;
    bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width = 2048;
    bufferCopyRegion.imageExtent.height = 2048;
    bufferCopyRegion.imageExtent.depth = 1;
    bufferCopyRegion.bufferOffset = 0;
    vkCmdCopyBufferToImage(
        copyCmd,
        stagingBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferCopyRegion);
    FnCommand::endSingleTimeCommand(device, commandPool, queue, copyCmd);

    /*
    auto copyCmd = FnCommand::beginSingleTimeCommand(device, commandPool);
    // Copy mip levels from staging buffer
    vkCmdCopyBufferToImage(
        copyCmd,
        stagingBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        bufferCopyRegions.size(),
        bufferCopyRegions.data());
    FnCommand::endSingleTimeCommand(device, commandPool, queue, copyCmd);*/

    //5. image layout from DST->ShaderRead
    FnImage::transitionImageLayout(device, commandPool, queue,image,
                           format,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1,1);


    FnVmaBuffer::destroyBuffer(allocator, stagingBuffer, stagingAllocation);
    ktxTexture_Destroy(ktx);


    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descImageInfo.imageView = view;
    descImageInfo.sampler = sampler;
}

void VmaUBOKTX2Texture::create(const std::string &file, VkSampler sampler) {
    std::cout << "[[VmaUBOKTX2Texture::create]] read:" << file << std::endl;
    const auto &[device,physicalDevice,commandPool, queue, allocator] = requiredObjects;

    assert(device != VK_NULL_HANDLE && physicalDevice!=VK_NULL_HANDLE && commandPool!=VK_NULL_HANDLE);
    ktxTexture* kTexture;
    ktxTexture2* kTexture2{nullptr};
    KTX_error_code result;
    ktxVulkanDeviceInfo vdi;


    // Set up Vulkan physical device (gpu), logical device (device), queue
    // and command pool. Save the handles to these in a struct called vkctx.
    // ktx VulkanDeviceInfo is used to pass these with the expectation that
    // apps are likely to upload a large number of textures.
    ktxVulkanDeviceInfo_Construct(&vdi, physicalDevice, device,
                                  queue, commandPool, nullptr);

    result = ktxTexture_CreateFromNamedFile(file.c_str(),
                                               KTX_TEXTURE_CREATE_NO_FLAGS,
                                               &kTexture);
    if (result != KTX_SUCCESS)
        throw std::runtime_error(std::format("failed to load textures:{}",file));

    if(kTexture->classId == ktxTexture2_c)
        kTexture2 = reinterpret_cast<ktxTexture2*>(kTexture); // if ktx->classId == ktxTexture2_c

    if(kTexture2 and ktxTexture2_NeedsTranscoding(kTexture2)) {
        ktx_texture_transcode_fmt_e tf;

        // Using VkGetPhysicalDeviceFeatures or GL_COMPRESSED_TEXTURE_FORMATS or
        // extension queries, determine what compressed texture formats are
        // supported and pick a format. For example
        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

        khr_df_model_e colorModel = ktxTexture2_GetColorModel_e(kTexture2);
        std::cout << "[[VmaUBOKTX2Texture::create]] find need transcode:" <<magic_enum::enum_name(colorModel) << std::endl;
        if (colorModel == KHR_DF_MODEL_UASTC and deviceFeatures.textureCompressionASTC_LDR) {
            tf = KTX_TTF_ASTC_4x4_RGBA;
        }
        else if (colorModel == KHR_DF_MODEL_ETC1S && deviceFeatures.textureCompressionETC2) {
            tf = KTX_TTF_ETC;
        } else if (deviceFeatures.textureCompressionASTC_LDR) {
            tf = KTX_TTF_ASTC_4x4_RGBA;
        } else if (deviceFeatures.textureCompressionETC2)
            tf = KTX_TTF_ETC2_RGBA;
        else if (deviceFeatures.textureCompressionBC)
            tf = KTX_TTF_BC3_RGBA;
        else {
            auto er = std::format("current texture encode mode:{}, gpu-device support textureCompressionASTC_LDR:{}",  magic_enum::enum_name(colorModel) ,
                deviceFeatures.textureCompressionASTC_LDR);
            throw std::runtime_error{er};
        }
        std::cout << "[[VmaUBOKTX2Texture::create::file tanscoding]]" << magic_enum::enum_name(tf)  << std::endl;
        result = ktxTexture2_TranscodeBasis(kTexture2, tf, 0);
        assert(result == KTX_SUCCESS);
    }

    assert(KTX_SUCCESS == result) ;
    result = ktxTexture_VkUploadEx(kTexture, &vdi, &ktx_vk_texture,
                                      VK_IMAGE_TILING_OPTIMAL,
                                      VK_IMAGE_USAGE_SAMPLED_BIT,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(KTX_SUCCESS == result) ;
    image = ktx_vk_texture.image;

    VkImageViewCreateInfo viewCreateInfo = FnImage::imageViewCreateInfo(image, ktx_vk_texture.imageFormat);
    viewCreateInfo.format = ktx_vk_texture.imageFormat;
    viewCreateInfo.viewType =ktx_vk_texture.viewType;
    viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCreateInfo.subresourceRange.layerCount = ktx_vk_texture.layerCount;
    viewCreateInfo.subresourceRange.levelCount = ktx_vk_texture.levelCount;
    FnImage::createImageView(device,viewCreateInfo, view);
    std::cout <<"[[VmaUBOKTX2Texture::create image view]]"  <<magic_enum::enum_name(ktx_vk_texture.imageFormat) << " view type:" << magic_enum::enum_name(ktx_vk_texture.viewType) << std::endl; // VK_FORMAT_BC3_UNORM_BLOCK
    ktxTexture_Destroy(kTexture);
    ktxVulkanDeviceInfo_Destruct(&vdi);

    descImageInfo.sampler = sampler;
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descImageInfo.imageView = view;
}


void VmaUBOKTX2Texture::cleanup() {
    // When done using the image in Vulkan...
    ktxVulkanTexture_Destruct(&ktx_vk_texture, requiredObjects.device, nullptr);
    vkDestroyImageView(requiredObjects.device, view, nullptr);
}






LLVK_NAMESPACE_END


