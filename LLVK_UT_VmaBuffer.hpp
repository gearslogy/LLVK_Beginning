//
// Created by liuya on 8/25/2024.
//

#ifndef LLVK_UT_VMABUFFER_H
#define LLVK_UT_VMABUFFER_H

#include "LLVK_VmaBuffer.h"
#include "LLVK_GeomtryLoader.h"
LLVK_NAMESPACE_BEGIN

namespace UT_VmaBuffer {
    constexpr VkBufferUsageFlags vertex_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkBufferUsageFlags index_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    inline constexpr auto getGLTFVerticesSize = [](const GLTFLoader &geom, int32_t partIdx, VkBufferUsageFlags usage) {
        if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
            // Create for plant
            size_t vertexBufferSize = sizeof(GLTFVertex) * geom.parts[partIdx].vertices.size();
            return vertexBufferSize;
        }
        if (usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
            size_t indexBufferSize = sizeof(uint32_t) * geom.parts[partIdx].indices.size();
            return indexBufferSize;
        }
        assert(false);
        return static_cast<size_t>(0);
    };

    inline void addGeometryToSimpleBufferManager(GLTFLoader &geom, VmaSimpleGeometryBufferManager &manager) {
        for(int i=0;i<geom.parts.size(); i++) {
            size_t vertex_mem_size = getGLTFVerticesSize(geom, i, vertex_usage);
            size_t index_mem_size = getGLTFVerticesSize(geom, i, index_usage);
            manager.createBufferWithStagingBuffer<vertex_usage>(vertex_mem_size, geom.parts[i].vertices.data() );
            manager.createBufferWithStagingBuffer<index_usage>(index_mem_size, geom.parts[i].indices.data() );
            geom.parts[i].verticesBuffer = manager.createVertexBuffers.back().buffer;
            geom.parts[i].indicesBuffer = manager.createIndexedBuffers.back().buffer;
        }
    }
    struct RenderGLTFRequiredObjects {
        const GLTFLoader *geo;
        uint32_t part;
        VkPipelineLayout layout;
        VkCommandBuffer commandBuffer;
    };

    /*
    inline void renderGLTFGeo(const RenderGLTFRequiredObjects &requiredData) {
        const auto &cmdBuf = requiredData.commandBuffer;
        const auto &geo = requiredData.geo;
        //const auto &partVerticesBuffer =
        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &gridGeo.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,gridGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipelineLayout, 0, 1, &sceneSets.opaque, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, gridGeo.parts[0].indices.size(), 1, 0, 0, 0);
    }*/


}





LLVK_NAMESPACE_END


#endif //LLVK_UT_VMABUFFER_H
