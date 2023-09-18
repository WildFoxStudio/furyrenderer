set(SOURCES 
  # Utils
  "utilities/WindowFixture.h"
  "utilities/ExtractBuffer.h"
  # Unit
  "unit/RICacheMap.test.cpp"
  "unit/RingBufferManager.test.cpp"
  "unit/vulkan/VkUtils.test.cpp"
  "unit/vulkan/RenderPassCaching.test.cpp"
  "unit/vulkan/RIRenderPassAttachmentsConversion.test.cpp"
  # Integration
  "integration/vulkan/ContextCreationDestruction.test.cpp"
  "integration/vulkan/SwapchainCreationDestruction.test.cpp"
  "integration/vulkan/VertexBufferCreationDestruction.test.cpp"
  "integration/vulkan/VertexBufferUpload.test.cpp"
  "integration/vulkan/UniformBufferUpload.test.cpp"
)



include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(tests
"main.cpp"
)

target_sources(tests PRIVATE ${GTEST_DIR} ${GTEST_DIR_INCLUDE}${SOURCES})

target_link_libraries(tests FoxFury GTest::gtest_main GTest::gmock volk VulkanMemoryAllocator glfw)

set_target_properties(tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
)

include_directories(${LIB_INCLUDE_DIR} "utilities")
