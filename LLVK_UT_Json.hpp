//
// Created by lp on 2024/9/19.
//

#ifndef LLVK_UT_JSON_HPP
#define LLVK_UT_JSON_HPP
#include <libs/json.hpp>
namespace nlohmann {
    template <>
      struct adl_serializer<glm::vec2> {
        static void to_json(json& j, const glm::vec2 & vec) {
            j = {vec.x, vec.y };
        }
        static void from_json(const json& j, glm::vec2& vec) {
            j[0].get_to(vec.x);
            j[1].get_to(vec.y);
        }
    };

    template <>
    struct adl_serializer<glm::vec3> {
        static void to_json(json& j, const glm::vec3 & vec) {
            j = {vec.r, vec.g, vec.b };
        }
        static void from_json(const json& j, glm::vec3& vec) {
            j[0].get_to(vec.x);
            j[1].get_to(vec.y);
            j[2].get_to(vec.z);
        }
    };

    template <>
      struct adl_serializer<glm::vec4> {
        static void to_json(json& j, const glm::vec4 & vec) {
            j = {vec.x, vec.y, vec.z, vec.w };
        }
        static void from_json(const json& j, glm::vec4& vec) {
            j[0].get_to(vec.x);
            j[1].get_to(vec.y);
            j[2].get_to(vec.z);
            j[3].get_to(vec.w);
        }
    };


}
#endif //LLVK_UT_JSON_HPP
