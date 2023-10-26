cmake_minimum_required(VERSION 3.26.4)
project(Rendering CXX)

add_executable(${PROJECT_NAME}
"../App.h"
"../App.cpp"
"Rendering.cpp"
)


target_link_libraries(${PROJECT_NAME} PRIVATE FoxFury glfw glm gli)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
)

#Set the debug working directory
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${LIB_INCLUDE_DIR} ${CGLTF_INCLUDE_DIR})