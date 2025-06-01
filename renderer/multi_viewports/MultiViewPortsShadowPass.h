//
// Created by liuya on 4/25/2025.
//

#ifndef MULTIVIEWPORTSSHADOWPASS_H
#define MULTIVIEWPORTSSHADOWPASS_H

#include "renderer/public/SimpleShadowPass.h"

LLVK_NAMESPACE_BEGIN
class MultiViewPorts;
struct MultiViewPortsShadowPass final : SimpleShadowPass {
public:
    explicit MultiViewPortsShadowPass(const MultiViewPorts *renderer);
    void drawObjects() override;
};

LLVK_NAMESPACE_END

#endif //MULTIVIEWPORTSSHADOWPASS_H
