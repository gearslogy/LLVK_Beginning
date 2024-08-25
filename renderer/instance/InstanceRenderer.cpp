//
// Created by liuya on 8/16/2024.
//

#include "InstanceRenderer.h"

LLVK_NAMESPACE_BEGIN
void InstanceRenderer::cleanupObjects() {

}

void InstanceRenderer::loadTexture() {

}

void InstanceRenderer::loadModel() {
    Geos.plantGeo.load("content/plants/gardenplants/var0.gltf");
    Geos.groundGeo.load("content/ground/ground.gltf");

    constexpr VkBufferUsageFlags vertex_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkBufferUsageFlags index_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    auto getGLTFVerticesSize = [](const GLTFLoader &geom, int32_t partIdx, VkBufferUsageFlags usage) {
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
    const auto plantVertexBufferSize = getGLTFVerticesSize(Geos.plantGeo, 0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    const auto plantIndexBufferSize = getGLTFVerticesSize(Geos.plantGeo, 0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    Geos.bufferManager.createBufferWithStagingBuffer<vertex_usage>(plantVertexBufferSize, Geos.plantGeo.parts[0].vertices.data());
    Geos.bufferManager.createBufferWithStagingBuffer<index_usage>(plantIndexBufferSize, Geos.plantGeo.parts[0].indices.data());
    Geos.plantGeo.parts[0].verticesBuffer =  Geos.bufferManager.createVertexBuffers.end()->buffer;
    Geos.plantGeo.parts[0].indicesBuffer =   Geos.bufferManager.createIndexedBuffers.end()->buffer;


    const auto groundVertexBufferSize = getGLTFVerticesSize(Geos.groundGeo, 0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    const auto groundIndexBufferSize = getGLTFVerticesSize(Geos.groundGeo, 0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    Geos.bufferManager.createBufferWithStagingBuffer<vertex_usage>(groundVertexBufferSize, Geos.groundGeo.parts[0].vertices.data());
    Geos.bufferManager.createBufferWithStagingBuffer<index_usage>(groundIndexBufferSize, Geos.groundGeo.parts[0].indices.data());
    Geos.groundGeo.parts[0].verticesBuffer =  Geos.bufferManager.createVertexBuffers.end()->buffer;
    Geos.groundGeo.parts[0].indicesBuffer =   Geos.bufferManager.createIndexedBuffers.end()->buffer;

}

void InstanceRenderer::setupDescriptors() {
}

void InstanceRenderer::preparePipelines() {
}

void InstanceRenderer::prepareUniformBuffers() {
}

void InstanceRenderer::updateUniformBuffers() {
}

void InstanceRenderer::bindResources() {
}

void InstanceRenderer::recordCommandBuffer() {

}

LLVK_NAMESPACE_END
