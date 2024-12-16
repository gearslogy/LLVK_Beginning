//
// Created by liuya on 12/8/2024.
//

#ifndef RBDVATRENDERER_H
#define RBDVATRENDERER_H



#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"
#include "LLVK_ExrImage.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_GeometryLoaderV2.hpp"
LLVK_NAMESPACE_BEGIN
class RbdVatRenderer :public VulkanRenderer {
public:
    RbdVatRenderer();
    void prepare() override;
    void cleanupObjects() override;
private:
    GLTFLoaderV2::Loader<GLTFVertexVATFracture> buildings{};
    VmaSimpleGeometryBufferManager geomManager{};
    // VAT
    VkSampler colorSampler{};
    VkSampler vatSampler{};
    VmaUBOExrRGBATexture texPosition;
    VmaUBOExrRGBATexture texRotation;

};
LLVK_NAMESPACE_END


#endif //RBDVATRENDERER_H
