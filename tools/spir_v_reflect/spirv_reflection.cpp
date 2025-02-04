//
// Created by liuya on 1/14/2025.
//

#include "spirv_reflection.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include "libs/magic_enum.hpp"
#include <vulkan/vulkan.h>

int main(int argn, char** argv) {
    if (argn != 2) {
        std::cerr << "Usage: ./spirv_reflection <filename>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string input_spv_path = argv[1];

    std::ifstream spv_ifstream(input_spv_path.c_str(), std::ios::binary);
    if (!spv_ifstream.is_open()) {
        std::cerr << "ERROR: could not open '" << input_spv_path << "' for reading\n";
        return EXIT_FAILURE;
    }
    spv_ifstream.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(spv_ifstream.tellg());
    spv_ifstream.seekg(0, std::ios::beg);

    std::vector<char> spv_data(size);
    spv_ifstream.read(spv_data.data(), size);

    // Generate reflection data for a shader
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spv_data.size(), spv_data.data(), &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    const auto stageStage = static_cast<VkShaderStageFlagBits>(module.shader_stage);
    std::cout << "shader stage:" <<magic_enum::enum_name(stageStage) << std::endl;
    std::cout << "entry points count:" << module.entry_point_count << std::endl;
    for (int i=0;i <module.entry_point_count;i++) {
        const auto entry_point = module.entry_points[i];
        std::cout << entry_point.name << std::endl;
    }

    spir_v_reflect::descriptorReflection(module);
    spir_v_reflect::IOReflection(module);
    spir_v_reflect::pushConstantReflection(module);
    spvReflectDestroyShaderModule(&module);
    std::cin.get();
    return 0;
}

void spir_v_reflect::descriptorReflection(const SpvReflectShaderModule &module) {
    uint32_t count = 0;
    SpvReflectResult result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::cout << "descriptor set count: " << count << std::endl;


    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);


    for (const auto &set: sets) {
        std::cout << std::format(">>>>  set id:{}, binding count:{}", set->set, set->binding_count) <<std::endl;
        for (int bindingIndex=0; bindingIndex < set->binding_count ;bindingIndex++) {
            const SpvReflectDescriptorBinding *pBinding = set->bindings[bindingIndex];


            const uint32_t bindingPosition = pBinding->binding ;
            const auto descType = static_cast<VkDescriptorType>(pBinding->descriptor_type);
            uint32_t descriptorCount{1};
            for (uint32_t i_dim = 0; i_dim < pBinding->array.dims_count; ++i_dim) {
                descriptorCount *= pBinding->array.dims[i_dim];
            }
            const char * bindingName = pBinding->name;
            const bool accessed = pBinding->accessed;

            const auto dumpInfo = std::format("\t----{}, binding:{}, descCount:{} type:{}, accessed:{}----", bindingName,
                bindingPosition, descriptorCount,
                magic_enum::enum_name(descType), accessed );
            std::cout << dumpInfo << std::endl;

            if (pBinding->uav_counter_binding != nullptr) {
                std::stringstream os;
                os << "\t" << "counter  : ";
                os << "(";
                os << "set=" << pBinding->uav_counter_binding->set << ", ";
                os << "binding=" << pBinding->uav_counter_binding->binding << ", ";
                os << "name=" << pBinding->uav_counter_binding->name;
                os << ");";
                os << "\n";
                std::cout << os.str();
            }


            if(pBinding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER or pBinding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                parseBlock(pBinding->block , 1);
            }


            /*
            std::cout << "\t\tmembers cout :" << memberCount<< std::endl;

            for (int m = 0; m < memberCount; ++m) {
                const auto &member = pTypeDescription->members[i];
                const char *memName = member.struct_member_name ;
                std::cout << std::format("\t\t:{}", memName) << std::endl;
            }
            if (memberCount !=0) {
                const SpvReflectBlockVariable &block = pBinding->block;
                auto blockInfo = std::format("\t\tbinding block-> offset:{} abs_offset:{} padded_size:{}", block.offset, block.absolute_offset, block.padded_size);
                std::cout << blockInfo << std::endl;
            }*/


        }
    }
}
void spir_v_reflect::parseBlock(const SpvReflectBlockVariable &block, int indent) {
    auto getTableStr = [](int count) {
        std::stringstream ss;
        for (int i=0;i<count;i++)
            ss<<"\t";
        return ss.str();
    };


    const uint32_t member_count = block.member_count; // pTypeDescription->member_count ; // this ok
    // BLOCK BEGIN
    {
        const char *type_name = (block.type_description->type_name != nullptr) ? block.type_description->type_name : "<unnamed>";
        const auto size = block.size;
        const auto padded_size = block.padded_size;
        const auto glslType = toStringGlslType(*block.type_description);
        std::cout << std::format("{}<<block begin ->name:{} typename:{} glsl_t:{} size:{} padded_size:{}>>",
            getTableStr(indent),
            block.name,
            type_name,
            glslType,
            size, padded_size) << std::endl;
    }
    if (member_count == 0) {
        return;
    }
    std::cout << getTableStr(indent) <<"{" << std::endl;
    const SpvReflectBlockVariable* p_members = block.members;
    for (uint32_t member_index = 0; member_index < member_count; ++member_index) {
        const SpvReflectBlockVariable &member = p_members[member_index];
        if (!member.type_description) {
            continue;
        }
        bool is_struct = ((member.type_description->type_flags & static_cast<SpvReflectTypeFlags>(SPV_REFLECT_TYPE_FLAG_STRUCT)) != 0);
        bool is_ref = ((member.type_description->type_flags & static_cast<SpvReflectTypeFlags>(SPV_REFLECT_TYPE_FLAG_REF)) != 0);
        bool is_array = ((member.type_description->type_flags & static_cast<SpvReflectTypeFlags>(SPV_REFLECT_TYPE_FLAG_ARRAY)) != 0);
        const bool is_array_struct = is_array && member.type_description->struct_type_description;
        const char *memberName = member.name;
        const char *typeName = (member.type_description->type_name == nullptr ? "<not struct>" : member.type_description->type_name);
        const auto absolute_offset = member.absolute_offset;
        const auto relative_offset = member.offset;
        const auto size = member.size;
        const auto padded_size = member.padded_size;
        const auto array_stride = member.array.stride;
        const auto block_variable_flags = member.flags;

        const auto subMemberCount = member.member_count;

        std::cout << std::format("{}name:{}, type:{} abs_offset:{} ral_offset:{} size:{} padded_size:{} array_stride:{} var_flags:{} is_struct:{} is_array:{} is_array_struct:{} sub_member_count:{}",
            getTableStr(indent),
            memberName, typeName,
            absolute_offset, relative_offset,
            size, array_stride,
            padded_size,
            magic_enum::enum_name(static_cast<SpvReflectVariableFlagBits>(block_variable_flags)),
            is_struct,
            is_array, is_array_struct, subMemberCount);

        if (is_array) {
            std::cout << " array-dim:"<< member.array.dims_count ;
            for (int dimIdx =0 ; dimIdx <member.array.dims_count; dimIdx++) {
                std::cout << "  ["<< dimIdx <<"]="<<member.array.dims[dimIdx];
            }
        }
        std::cout << "\n";
        if (is_struct and subMemberCount !=0){
            for (auto nextMemberIdx = 0 ; nextMemberIdx < subMemberCount; nextMemberIdx++) {
                parseBlock(member.members[nextMemberIdx] , indent+1);
                //parseBlock(member.members[nextMemberIdx] );
                //nextMember = member.members[nextMemberIdx];
            }
        }
    }
    std::cout << getTableStr(indent)<<"}" << std::endl;


}

void spir_v_reflect::IOReflection(const SpvReflectShaderModule &module) {

    auto buildInSpvVarStr = [](this auto self, const SpvReflectInterfaceVariable &var) {
        std::stringstream ss;
        if (var.decoration_flags & SPV_REFLECT_DECORATION_BLOCK) {
            ss << "(built-in block)";
        }else {
            ss << "(built-in var) ";
        }
        ss << "[";
        for (uint32_t i = 0; i < var.member_count; i++) {
            ss << magic_enum::enum_name(var.members[i].built_in);
            if (i < (var.member_count - 1)) {
                ss << ", ";
            }
        }
        ss << "]";
        return ss.str();
    };

    auto dumpVariable = [buildInSpvVarStr](const SpvReflectInterfaceVariable &var) {
        bool isBuildIn = var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN;
        std::string buildInStr = isBuildIn ?  buildInSpvVarStr(var) : "";
        const auto dims_count = var.array.dims_count;
        const auto *name = var.name ? var.name : "";
        const auto *semantic = var.semantic ? var.semantic : "";
        const auto glsl_t = toStringGlslType (  *var.type_description);
        std::cout << std::format(" location:{}\n name:{}\n buidin:{}\n dims_count:{}\n semantic:{}\n glsl_t:{}\n", var.location, name, buildInStr, dims_count, semantic, glsl_t);
    };

    std::cout << "input variables:" << module.input_variable_count << std::endl;
    uint32_t inputVariableCount = module.input_variable_count;
    for (int i=0;i<inputVariableCount;i++) {
        const SpvReflectInterfaceVariable *var = module.input_variables[i];
        std::cout << "--------\n";
        dumpVariable(*var);
    }

    std::cout << "\noutput variables:" << module.output_variable_count << std::endl;
    // output
    uint32_t outputVariableCount = module.output_variable_count;
    for (int i=0;i<outputVariableCount;i++) {
        const SpvReflectInterfaceVariable *var = module.output_variables[i];
        std::cout << "--------\n";
        dumpVariable(*var);
    }

}

void spir_v_reflect::pushConstantReflection(const SpvReflectShaderModule &module) {
    std::cout << "\ndump push constant:" << std::endl;
    for (int i=0;i<module.push_constant_block_count;i++) {
       SpvReflectBlockVariable blockVar =  module.push_constant_blocks[i];
        parseBlock(blockVar, 0);
    }
}

