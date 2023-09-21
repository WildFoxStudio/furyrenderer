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

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME} STATIC
   "include/main.cpp"
)

add_subdirectory("thirdparty/volk")
add_subdirectory("thirdparty/VulkanMemoryAllocator")
add_subdirectory("thirdparty/glfw")

target_link_libraries(${PROJECT_NAME} PRIVATE volk VulkanMemoryAllocator)

# Set custom output directories - to have same behaviour accross different compilers
# On linux with make it doesn't work, no debug or release folders
set(OUTPUTDIR "${CMAKE_CURRENT_SOURCE_DIR}/binaries") 
# First for the generic no-config case (e.g. with mingw)
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUTDIR} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUTDIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUTDIR} )
# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUTDIR} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUTDIR} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUTDIR} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${OUTPUTDIR}
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUTDIR}
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUTDIR}
)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR} ${SRC_DIR})

target_sources(${PROJECT_NAME} PRIVATE 
    "${INCLUDE_DIR}/main.cpp"
    "${INCLUDE_DIR}/IContext.h"
    "${INCLUDE_DIR}/backend/vulkan/VulkanContextFactory.h"
    "${SRC_DIR}/backend/vulkan/VulkanContextFactory.cpp"
    "${SRC_DIR}/Asserts.h"
    "${SRC_DIR}/RICacheMap.h"
    "${SRC_DIR}/backend/vulkan/UtilsVk.h"
    "${SRC_DIR}/RingBufferManager.h"
    "${SRC_DIR}/backend/vulkan/ResourceTransfer.h"
    "${SRC_DIR}/backend/vulkan/ResourceTransfer.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanContext.h"
    "${SRC_DIR}/backend/vulkan/VulkanContext.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanInstance.h"
    "${SRC_DIR}/backend/vulkan/VulkanInstance.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice2.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice2.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice3.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice3.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice4.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice4.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice5.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice5.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice6.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice6.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice7.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice7.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice8.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice8.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice9.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice9.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice10.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice10.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice11.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice11.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice12.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice12.cpp"
    "${SRC_DIR}/backend/vulkan/VulkanDevice13.h"
    "${SRC_DIR}/backend/vulkan/VulkanDevice13.cpp"
    )


    
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)

if(BUILD_EXAMPLES)
    set(LIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
    add_subdirectory("examples/HelloTriangle")
    add_subdirectory("examples/SpinningTriangle")
endif()

if(BUILD_TESTS)
add_subdirectory("tests")
endif()