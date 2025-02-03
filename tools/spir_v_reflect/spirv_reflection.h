//
// Created by liuya on 1/14/2025.
//

#ifndef SPIR_V_REFLECT_H
#define SPIR_V_REFLECT_H

#include "spirv_reflect.h"


struct spir_v_reflect {
    static void descriptorReflection(const SpvReflectShaderModule &module);
    static void parseBlock(const SpvReflectBlockVariable &block, int ident= 0 );


    static std::string toStringScalarType(const SpvReflectTypeDescription& type) {
        switch (type.op) {
            case SpvOpTypeVoid: {
                return "void";
                break;
            }
            case SpvOpTypeBool: {
                return "bool";
                break;
            }
            case SpvOpTypeInt: {
                if (type.traits.numeric.scalar.signedness)
                    return "int";
                else
                    return "uint";
            }
            case SpvOpTypeFloat: {
                switch (type.traits.numeric.scalar.width) {
                    case 32:
                        return "float";
                    case 64:
                        return "double";
                    default:
                        break;
                }
                break;
            }
            case SpvOpTypeStruct: {
                return "struct";
            }
            case SpvOpTypePointer: {
                return "ptr";
            }
            default: {
                break;
            }
        }
        return "";
    }

    static std::string toStringGlslType(const SpvReflectTypeDescription& type) {
        switch (type.op) {
            case SpvOpTypeVector: {
                switch (type.traits.numeric.scalar.width) {
                    case 32: {
                        switch (type.traits.numeric.vector.component_count) {
                            case 2:
                                return "vec2";
                            case 3:
                                return "vec3";
                            case 4:
                                return "vec4";
                        }
                    } break;

                    case 64: {
                        switch (type.traits.numeric.vector.component_count) {
                            case 2:
                                return "dvec2";
                            case 3:
                                return "dvec3";
                            case 4:
                                return "dvec4";
                            default: return "unknown";
                        }
                    } break;
                }
            } break;
            default:
                break;
        }
        return toStringScalarType(type);
    }
};



#endif //SPIR_V_REFLECT_H
