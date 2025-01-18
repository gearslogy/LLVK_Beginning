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

    spvReflectDestroyShaderModule(&module);
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
        std::cout << std::format("set id:{}, binding count:{}", set->set, set->binding_count) <<std::endl;
        for (int i=0;i< set->binding_count ;i++) {
            const auto *pBinding = set->bindings[i];


            const uint32_t bindingPosition = pBinding->binding ;
            const auto descType = static_cast<VkDescriptorType>(pBinding->descriptor_type);
            uint32_t descriptorCount{1};
            for (uint32_t i_dim = 0; i_dim < pBinding->array.dims_count; ++i_dim) {
                descriptorCount *= pBinding->array.dims[i_dim];
            }
            const char * bindingName = pBinding->name;
            const bool accessed = pBinding->accessed;

            const auto dumpInfo = std::format("\t----{}, binding:{}, descCount:{} {}, accessed:{}----", bindingName,
                bindingPosition, descriptorCount,
                magic_enum::enum_name(descType), accessed );
            std::cout << dumpInfo << std::endl;

            //std::cout << magic_enum::enum_name(static_cast<SpvReflectDecorationFlagBits>(pBinding->decoration_flags)) << std::endl;

            const auto *pTypeDescription = pBinding->type_description;
            const auto *typeName = pTypeDescription->type_name ;
            if (typeName != nullptr) {
                const auto dumpTypeInfo = std::format("\ttype:{}", typeName );
                std::cout << dumpTypeInfo << std::endl;
            }

            const uint32_t memberCount = pTypeDescription->member_count ;
            std::cout << "\tmembers cout :" << memberCount<< std::endl;

            for (int m = 0; m < memberCount; ++m) {
                const char *memName = pTypeDescription->members[m].struct_member_name ;
                std::cout << std::format("\t\t{}", memName) << std::endl;
            }

            const SpvReflectBlockVariable &block = pBinding->block;


        }
    }
}
