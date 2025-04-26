//
// Created by liuya on 4/25/2025.
//

#ifndef MULTIVIEWPORTSSHADOWPASS_H
#define MULTIVIEWPORTSSHADOWPASS_H

#include "renderer/subpass/SPShadowPass.h"

LLVK_NAMESPACE_BEGIN
class MultiViewPortsShadowPass : public SPShadowPass {
public:
    void recordCommandBuffer() override;
};

LLVK_NAMESPACE_END

#endif //MULTIVIEWPORTSSHADOWPASS_H
