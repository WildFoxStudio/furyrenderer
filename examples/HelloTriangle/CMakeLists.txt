cmake_minimum_required(VERSION 3.26.4)
project(HelloTriangle CXX)

add_executable(${PROJECT_NAME}
"HelloTriangle.cpp"
)


target_link_libraries(${PROJECT_NAME} PRIVATE FoxFury)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
)

include_directories(${LIB_INCLUDE_DIR})