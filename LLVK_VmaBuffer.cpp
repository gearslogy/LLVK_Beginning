//
// Created by liuya on 8/4/2024.
//

#include "LLVK_vmaBuffer.h"


LLVK_NAMESPACE_BEGIN
void VmaUBOBuffer::createAndMapping(VkDeviceSize bufferSize) {
    VkBuffer stagingBuffer{};
    VmaAllocation stagingAllocation{};
    VkResult result = FnVmaBuffer::createBuffer<true>(requiredObjects.device,
        requiredObjects.allocator,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        stagingBuffer, stagingAllocation);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR create stagging vma buffer"};

    vmaMapMemory(requiredObjects.allocator, stagingAllocation, &mapped);
}
void VmaUBOBuffer::cleanup() {
    FnVmaBuffer::destroyBuffer(requiredObjects.device, requiredObjects.allocator ,bufferAndAllocation.buffer,bufferAndAllocation.allocation );
}


LLVK_NAMESPACE_END


