//
// Created by liuya on 7/27/2024.
//

#pragma once

#include "LLVK_Utils.hpp"
#include "LLVK_Image.h"
#include "BufferManager.h"

LLVK_NAMESPACE_BEGIN
inline void* alignedAlloc(size_t size, size_t alignment)
{
    void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
    data = _aligned_malloc(size, alignment);
#else
    int res = posix_memalign(&data, alignment, size);
    if (res != 0)
        data = nullptr;
#endif
    return data;
}

inline void alignedFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
    _aligned_free(data);
#else
    free(data);
#endif
}


namespace FnDescriptor {
    // (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT, arrayCount);
    inline constexpr VkDescriptorSetLayoutBinding setLayoutBinding(VkDescriptorType type, uint32_t binding,VkShaderStageFlags stageBit, uint32_t arrayCount= 1) {
        VkDescriptorSetLayoutBinding ret{};
        ret.binding = binding;
        ret.descriptorCount = arrayCount; // descriptorCount specifies the number of values in the array
        ret.descriptorType = type;
        ret.stageFlags = stageBit;
        ret.pImmutableSamplers = nullptr;
        return ret;
    }


    inline VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo(
        const VkDescriptorSetLayoutBinding* pBindings,
        uint32_t bindingCount){
        VkDescriptorSetLayoutCreateInfo ret {};
        ret.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ret.pBindings = pBindings;
        ret.bindingCount = bindingCount;
        return ret;
    }

    inline VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo(const Concept::is_range auto &bindings){
        VkDescriptorSetLayoutCreateInfo ret {};
        ret.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ret.pBindings = bindings.data();
        ret.bindingCount = bindings.size();
        return ret;
    }


    inline VkDescriptorPoolCreateInfo poolCreateInfo(
        const Concept::is_range auto&poolSizes,
        uint32_t maxSets)
    {
        VkDescriptorPoolCreateInfo ret{};
        ret.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ret.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        ret.pPoolSizes = poolSizes.data();
        ret.maxSets = maxSets;
        return ret;
    }
    inline VkDescriptorSetAllocateInfo setAllocateInfo(
                VkDescriptorPool descriptorPool,
                const VkDescriptorSetLayout* pSetLayouts,
                uint32_t descriptorSetCount)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
        descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
        return descriptorSetAllocateInfo;
    }

    inline VkDescriptorSetAllocateInfo setAllocateInfo(VkDescriptorPool descriptorPool,
        const Concept::is_range auto & layouts){
        return setAllocateInfo(descriptorPool, layouts.data(), layouts.size());
    }


    inline VkWriteDescriptorSet writeDescriptorSet(const VkDescriptorSet &dstSet,
        VkDescriptorType type,
        uint32_t binding,
        const VkDescriptorBufferInfo* bufferInfo,
        uint32_t descriptorCount = 1){
        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pBufferInfo = bufferInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }

    inline VkWriteDescriptorSet writeDescriptorSet(
        const VkDescriptorSet &dstSet,
        VkDescriptorType type,
        uint32_t binding,
        const VkDescriptorImageInfo *imageInfo,
        uint32_t descriptorCount = 1){
        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pImageInfo = imageInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }

}

struct UBORequiredObjects {
    VkDevice device{};
    VkPhysicalDevice physicalDevice{};
    VkCommandPool commandPool{};
    VkQueue queue{};
};


struct UBOBuffer {
    VkBuffer buffer{}; // ubo VS buffer
    VkDeviceMemory memory{};
    void * mapped{};
    VkDeviceSize memorySize{};
    VkDescriptorBufferInfo descBufferInfo{};

    UBORequiredObjects requiredObjs{};

    void create(VkDeviceSize bufferSize, VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        FnBuffer::createBuffer(requiredObjs.physicalDevice, requiredObjs.device, bufferSize,
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer,
                               memory);
        memorySize = bufferSize;
        descBufferInfo.buffer = buffer;
        descBufferInfo.offset = 0;

        descBufferInfo.range = bufferSize; // in dynamic ubo, it't should be per block size. but default give it buffersize
        //plantUniformBuffers.dynamicBuffer.descBufferInfo.range = dynamicAlignment;

    }
    void cleanup() {
        vkDestroyBuffer(requiredObjs.device, buffer, nullptr);
        vkFreeMemory(requiredObjs.device, memory, nullptr);
    }
    void map() {
        if(vkMapMemory(requiredObjs.device, memory, 0, memorySize, 0, &mapped) != VK_SUCCESS)
            throw std::runtime_error{"ERROR map memory"};
    }
};

struct UBOTexture { // this interface should be dropped. because vma texture is future
    ImageAndMemory imageAndMemory{};
    VkImageView imageView{};
    VkDescriptorImageInfo descImageInfo{}; // for writeDescriptorSet

    UBORequiredObjects requiredObjs{};
    void create(const std::string &file, VkSampler sampler) {
        imageAndMemory = FnImage::createTexture(requiredObjs.physicalDevice,requiredObjs.device, requiredObjs.commandPool, requiredObjs.queue,file);
        FnImage::createImageView( requiredObjs.device, imageAndMemory.image,
            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, imageAndMemory.mipLevels,1, imageView);
        descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descImageInfo.imageView = imageView;
        descImageInfo.sampler = sampler;
    }

    void cleanup() {
        vkDestroyImage(requiredObjs.device,imageAndMemory.image, nullptr);
        vkFreeMemory(requiredObjs.device,imageAndMemory.memory, nullptr);
        vkDestroyImageView(requiredObjs.device, imageView, nullptr);
    }

};

namespace MetaDesc {
    // types
    struct UBO {}; // ubo
    struct CIS {}; // combined image sampler
    struct UBO_DYNAMIC{};
    struct SSBO{};
    struct SAMPLER{};
    struct INPUT_ATTACHMENT{};
    // concept checking
    template<typename T>
    concept is_desc_ubo = std::is_same_v<T, UBO> ;
    template<typename T>
    concept is_desc_cis = std::is_same_v<T, CIS> ;
    template<typename T>
    concept is_ubo_dynamic = std::is_same_v<T, UBO_DYNAMIC> ;
    template<typename T>
    concept is_ssbo = std::is_same_v<T, SSBO> ;
    template<typename T>
    concept is_sampler = std::is_same_v<T, SAMPLER> ;
    template<typename T>
    concept is_input_attachment = std::is_same_v<T, INPUT_ATTACHMENT> ;
    // check all interface
    template<typename T>
    concept is_desc_type = is_desc_ubo<T> or is_desc_cis<T> or is_ubo_dynamic<T> or is_sampler<T> or is_ssbo<T> or is_input_attachment<T>;

    template<typename binding_type_t, uint32_t binding_position, uint32_t usage>
    consteval auto getBinding() {
        static_assert(is_desc_type<binding_type_t>, "type not supported");;
        if constexpr (is_desc_ubo<binding_type_t>)
            return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding_position, usage);
        if constexpr (is_desc_cis<binding_type_t> )
            return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, binding_position, usage);
        if constexpr (is_sampler<binding_type_t>)
            return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, binding_position, usage);
        if constexpr (is_ubo_dynamic<binding_type_t>)
            return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, binding_position, usage);
        if constexpr (is_ssbo<binding_type_t>)
            return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding_position, usage);
        if constexpr (is_input_attachment<binding_type_t>)
            return FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, binding_position, usage);
    }

    // type: of binding types
    template<is_desc_type ... >
    struct desc_types_t {};
    // type: of binding position
    template<size_t ... >
    struct desc_binding_position_t{};
    // type: of binding usage
    template<size_t ... >
    struct desc_binding_usage_t{};


    // binding_types checking
    template<typename T>
    struct is_binding_types : std::false_type {};
    template<is_desc_type ... T>
    struct is_binding_types<desc_types_t<T...>> : std::true_type {};

    // binding_position checking
    template<typename T>
    struct is_binding_position : std::false_type {};
    template<size_t... positions>
    struct is_binding_position<desc_binding_position_t<positions...>> : std::true_type {};

    // binding_usage checking
    template<typename T>
    struct is_binding_usage : std::false_type {};
    template<size_t... usages>
    struct is_binding_usage<desc_binding_usage_t<usages...>> : std::true_type {};

    // ------ concept interface-----
    template<typename T>
    concept is_desc_types_t = is_binding_types<T>::value;
    template<typename T>
    concept is_binding_position_t = is_binding_position<T>::value;
    template<typename T>
    concept is_binding_usage_t = is_binding_usage<T>::value;

    // interface IMPL
    template<typename types_t, typename positions_t, typename usages_t >
    struct GenerateSetLayoutBindingsImpl;

    //partial specialization
    template<template<typename...> typename desc_types_pack, typename ... types_t,
             std::size_t... BindingPos, std::size_t... Usages>
    struct GenerateSetLayoutBindingsImpl<
        desc_types_pack<types_t...>,
        desc_binding_position_t<BindingPos...>,
        desc_binding_usage_t<Usages...>>
    {
        static consteval auto generate() {
            constexpr size_t type_count = sizeof...(types_t);
            std::array<VkDescriptorSetLayoutBinding, type_count> bindings = {
                getBinding<types_t, BindingPos, Usages>()...
            };
            return bindings;
        }
    };
    template<is_desc_types_t Types, is_binding_position_t BindingPos, is_binding_usage_t Usages>
    consteval auto generateSetLayoutBindings() {
        return GenerateSetLayoutBindingsImpl<Types, BindingPos, Usages>::generate();
    }
}


LLVK_NAMESPACE_END