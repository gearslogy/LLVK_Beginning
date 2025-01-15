//
// Created by liuya on 1/14/2025.
//

#include "spirv_reflection.h"
#include <cassert>
#include <fstream>
#include <iostream>

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

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::cout << "descriptor set count: " << count << std::endl;


    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    for (const auto &set: sets) {
        std::cout <<"set id:" <<set->set << std::endl;
        std::cout << "binding count:" << set->binding_count << std::endl;
        for (int i=0;i< set->binding_count ;i++) {
            auto binding = set->bindings[i]->binding ;
            std::cout << binding << std::endl;
        }

    }

    return 0;
}
