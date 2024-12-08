//
// Created by liuya on 12/8/2024.
//

#ifndef RBDVATRENDERER_H
#define RBDVATRENDERER_H


#include "LLVK_GeomtryLoader.h"#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"
LLVK_NAMESPACE_BEGIN
class RbdVatRenderer :public VulkanRenderer {
public:
    RbdVatRenderer();
    void prepare() override;
    void cleanupObjects() override;
private:
    GLTFLoader buildings{};
    VmaSimpleGeometryBufferManager geomManager{};

    // VAT
    VkSampler colorSampler{};
    VmaUBOKTX2Texture texPosition;
    VmaUBOKTX2Texture texRotation;

};
LLVK_NAMESPACE_END


#endif //RBDVATRENDERER_H
