cmake_minimum_required(VERSION 3.26.4)
project(DepthBuffer CXX)

add_executable(${PROJECT_NAME}
"DepthBuffer.cpp"
)


target_link_libraries(${PROJECT_NAME} PRIVATE FoxFury glfw glm)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
)

#Set the debug working directory
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${LIB_INCLUDE_DIR})