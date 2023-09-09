cmake_minimum_required(VERSION 3.26.4)
project(FoxFury CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SUPPRESS_REGENERATION true)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wshadow -Wconversion -pedantic)
endif()

set(BIN_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/bin)
set(LIB_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/lib)

option(BUILD_TESTS "Build tests" ON)

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME} SHARED
    "include/main.cpp"
)

add_subdirectory("thirdparty/volk")

target_link_libraries(${PROJECT_NAME} PRIVATE volk)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIB_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${LIB_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
)

target_include_directories(${PROJECT_NAME} PUBLIC INCLUDE_DIR)

target_sources(${PROJECT_NAME} PRIVATE 
    "${INCLUDE_DIR}/main.cpp"
    )
    