//
// Created by liuya on 4/25/2025.
//

#include "MultiViewPortsShadowPass.h"
#include "MultiViewPorts.h"
LLVK_NAMESPACE_BEGIN
MultiViewPortsShadowPass::MultiViewPortsShadowPass(const MultiViewPorts *renderer):SimpleShadowPass(renderer)
{
}

void MultiViewPortsShadowPass::drawObjects() {
    auto *renderer = static_cast<const MultiViewPorts*>(pRenderer);
    renderer->drawObjects();
}

LLVK_NAMESPACE_END