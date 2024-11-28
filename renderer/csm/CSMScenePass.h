//
// Created by liuya on 11/10/2024.
//

#ifndef CSMSCENEPASS_H
#define CSMSCENEPASS_H

#include "LLVK_SYS.hpp"

LLVK_NAMESPACE_BEGIN
class CSMRenderer;
struct CSMScenePass {
    explicit CSMScenePass(CSMRenderer *rd);
    void prepare();
    void cleanup();
    void recordCommandBuffer();
    CSMRenderer* pRenderer;


};

LLVK_NAMESPACE_END


#endif //CSMSCENEPASS_H
