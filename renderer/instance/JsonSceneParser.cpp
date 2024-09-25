//
// Created by lp on 2024/9/19.
//

#include "JsonSceneParser.h"
#include <fstream>
#include "LLVK_UT_Json.hpp"
LLVK_NAMESPACE_BEGIN
JsonSceneParser::JsonSceneParser(const std::string &path,const GLTFLoader &instanceGeo):loader{&instanceGeo}{
    std::ifstream in(path.data());
    if (!in.good())
        throw std::runtime_error{std::string{"Could not open file "} + path + "."};
    in >> jsHandle;
    if(not jsHandle.contains("points") )
        throw std::runtime_error{std::string{"Could not parse JSON file, no points key "} };
    const nlohmann::json &points = jsHandle[points];


    for (auto &data: points) {
        const auto &P = data["P"].get<glm::vec3>();
        const auto &orient = data["orient"].get<glm::vec4>();
        const auto &pscale = data["scale"].get<float>();
        instancePositions.emplace_back(P);
        instanceOrient.emplace_back(orient);
        instanceScale.emplace_back(pscale);
    }
}
void JsonSceneParser::render() {

}




LLVK_NAMESPACE_END