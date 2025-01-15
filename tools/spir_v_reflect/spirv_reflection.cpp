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
    return 0;
}
