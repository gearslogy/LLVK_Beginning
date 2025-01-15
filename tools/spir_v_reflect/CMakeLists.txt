


message("spriv_v_reflect source dir:" ${CMAKE_SOURCE_DIR})

file(TO_CMAKE_PATH "$ENV{VK_SDK_PATH}" VULKAN_SDK)

set(SPIRV_REFLECT_PATH "${VULKAN_SDK}/Source/SPIRV-Reflect")
if(NOT EXISTS "${SPIRV_REFLECT_PATH}")
    message(FATAL_ERROR "Could not find SPIRV-Reflect at: ${SPIRV_REFLECT_PATH}")
endif()

set(SPIRV_REFLECT_SOURCE "${SPIRV_REFLECT_PATH}/spirv_reflect.c")
if(NOT EXISTS "${SPIRV_REFLECT_SOURCE}")
    message(FATAL_ERROR "Could not find spirv_reflect.c at: ${SPIRV_REFLECT_SOURCE}")
endif()

add_executable(spirv_reflection spirv_reflection.cpp "${SPIRV_REFLECT_SOURCE}")
set_target_properties(spirv_reflection PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(spirv_reflection PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(spirv_reflection PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
target_include_directories(spirv_reflection PRIVATE ${CMAKE_SOURCE_DIR} ${SPIRV_REFLECT_PATH})
target_include_directories(spirv_reflection PRIVATE ${Vulkan_INCLUDE_DIRS}) # ktx include
target_compile_definitions(spirv_reflection PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL)