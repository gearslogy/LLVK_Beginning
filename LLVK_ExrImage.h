//
// Created by liuya on 12/16/2024.
//

#ifndef LLVK_EXRIMAGE_H
#define LLVK_EXRIMAGE_H


#include "LLVK_SYS.hpp"
#include "LLVK_VmaBuffer.h"

LLVK_NAMESPACE_BEGIN
struct VmaUBOExrRGBATexture: IVmaUBOTexture {
    VmaBufferRequiredObjects requiredObjects{};
    void create(const std::string &file, const VkSampler &sampler);
    void cleanup();
private:
    static void parseExrRGBAHeader(const std::string &file);
    static float* parseExrRGBAData(const std::string &file, int &width, int &height);
};
LLVK_NAMESPACE_END


#endif //LLVK_EXRIMAGE_H
