cmake_minimum_required(VERSION 3.26.4)
project(HelloTriangle CXX)

add_executable(${PROJECT_NAME}
"HelloTriangle.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
)