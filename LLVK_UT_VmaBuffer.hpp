//
// Created by liuya on 8/25/2024.
//

#ifndef LLVK_UT_VMABUFFER_H
#define LLVK_UT_VMABUFFER_H

LLVK_NAMESPACE_BEGIN

namespace UT_VmaBuffer {
    template<typename Loader>
    concept is_geometry_loader = requires(Loader loader){
        //requires std::is_same_v< std::remove_cvref_t<decltype(loader.parts)>, std::vector<typename Loader::part_t> >; // V2
        loader.parts; // support v1 & v2
    };

    inline constexpr VkBufferUsageFlags vertex_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    inline constexpr VkBufferUsageFlags index_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    inline constexpr auto getGLTFVerticesSize = []<is_geometry_loader Loader>(const Loader &geom, int32_t partIdx, VkBufferUsageFlags usage){
        using vertex_t = typename std::remove_cvref_t<decltype(geom)>::vertex_t;
        if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
            // Create for plant
            size_t vertexBufferSize = sizeof(vertex_t) * geom.parts[partIdx].vertices.size();
            return vertexBufferSize;
        }
        if (usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
            size_t indexBufferSize = sizeof(uint32_t) * geom.parts[partIdx].indices.size();
            return indexBufferSize;
        }
        return static_cast<size_t>(0);
    };

    inline void addGeometryToSimpleBufferManager(auto &geom, auto &manager)
    requires(is_geometry_loader<decltype(geom)>)
    {
        for(int i=0;i<geom.parts.size(); i++) {
            size_t vertex_mem_size = getGLTFVerticesSize(geom, i, vertex_usage);
            size_t index_mem_size = getGLTFVerticesSize(geom, i, index_usage);
            manager.createBufferWithStagingBuffer<vertex_usage>(vertex_mem_size, geom.parts[i].vertices.data() );
            manager.createBufferWithStagingBuffer<index_usage>(index_mem_size, geom.parts[i].indices.data() );
            geom.parts[i].verticesBuffer = manager.createVertexBuffers.back().buffer;
            geom.parts[i].indicesBuffer = manager.createIndexedBuffers.back().buffer;
        }
    }

}





LLVK_NAMESPACE_END


#endif //LLVK_UT_VMABUFFER_H
