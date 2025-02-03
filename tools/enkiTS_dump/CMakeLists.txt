

add_executable(enkiTS_dump enkiTS_dump.cpp ${CMAKE_SOURCE_DIR}/libs/enkiTS/TaskScheduler.cpp )

set_target_properties(enkiTS_dump PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(enkiTS_dump PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(enkiTS_dump PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)

message("exr_dump source dir:" ${CMAKE_SOURCE_DIR})
target_include_directories(enkiTS_dump PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(enkiTS_dump PRIVATE ${CMAKE_SOURCE_DIR}/libs/enkiTS)
target_include_directories(enkiTS_dump PRIVATE ${Vulkan_INCLUDE_DIRS}) # ktx include
target_compile_definitions(enkiTS_dump PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL)