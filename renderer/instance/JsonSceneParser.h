//
// Created by lp on 2024/9/19.
//

#ifndef JSONSCENEPARSER_H
#define JSONSCENEPARSER_H
#include <libs/json.hpp>
#include "LLVK_GeomtryLoader.h"

LLVK_NAMESPACE_BEGIN
class JsonSceneParser {
    



    public:
    explicit JsonSceneParser(const std::string &path, const GLTFLoader &instanceGeo);
    void render();
private:
    nlohmann::json jsHandle;
    const GLTFLoader *loader;
    std::vector<glm::vec3> instancePositions;
    std::vector<glm::vec4> instanceOrient;
    std::vector<float> instanceScale;
};
LLVK_NAMESPACE_END


#endif //JSONSCENEPARSER_H
