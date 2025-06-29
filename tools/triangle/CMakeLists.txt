
set(CMAKE_CXX_STANDARD 26)
message(aaaaa ${CMAKE_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64)
set(GLFW_PATH ${CMAKE_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64/)
set(GLFW_PATH_INCLUDE ${GLFW_PATH}/include)
set(GLFW_PATH_LIB ${GLFW_PATH}/lib-vc2022)



add_executable(triangle main.cpp)

set_target_properties(triangle PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(triangle PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(triangle PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)

message("gltf_dump source dir:" ${CMAKE_SOURCE_DIR})
target_include_directories(triangle PRIVATE ${CMAKE_SOURCE_DIR})

target_include_directories(triangle PRIVATE ${Vulkan_INCLUDE_DIRS} ${GLFW_PATH_INCLUDE})
target_compile_definitions(triangle PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(triangle PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL)
target_link_directories(triangle PUBLIC ${GLFW_PATH_LIB}) # ktx lib dir
target_link_libraries(triangle ${Vulkan_LIBRARIES} glfw3)