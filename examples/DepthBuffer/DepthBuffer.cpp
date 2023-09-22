// Copyright RedFox Studio 2022

#include <iostream>

#include "IContext.h"
#include "backend/vulkan/VulkanContextFactory.h"

#include <iostream>

#define GLFW_INCLUDE_NONE // makes the GLFW header not include any OpenGL or
// OpenGL ES API header.This is useful in combination with an extension loading library.
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <fstream>
#include <string>

void
Log(const char* msg)
{
    std::cout << msg << std::endl;
};

constexpr uint32_t imageWidth  = 32;
constexpr uint32_t imageHeight = 32;
// converter https://notisrac.github.io/FileToCArray/
// clang-format off
constexpr uint8_t image[] = {0x41, 0x3c, 0x38, 0xff, 0x3a, 0x48, 0x48, 0xff, 0x3a, 0x3f, 0x45, 0xff, 0x40, 0x41, 0x39, 0xff, 0x42, 0x42, 0x44, 0xff, 0x3f, 0x3d, 0x40, 0xff, 0x3c, 0x41, 0x44, 0xff, 0x42, 0x41, 0x3f, 0xff, 
  0x3b, 0x43, 0x46, 0xff, 0x47, 0x3e, 0x3f, 0xff, 0x45, 0x3c, 0x35, 0xff, 0x3b, 0x41, 0x3d, 0xff, 0x3c, 0x3f, 0x48, 0xff, 0x3b, 0x40, 0x43, 0xff, 0x44, 0x45, 0x3f, 0xff, 0x43, 0x3b, 0x38, 0xff, 
  0x3d, 0x3f, 0x3e, 0xff, 0x43, 0x42, 0x3d, 0xff, 0x44, 0x3f, 0x43, 0xff, 0x3d, 0x43, 0x41, 0xff, 0x40, 0x3d, 0x44, 0xff, 0x40, 0x40, 0x36, 0xff, 0x44, 0x40, 0x3f, 0xff, 0x3c, 0x42, 0x42, 0xff, 
  0x3d, 0x42, 0x3e, 0xff, 0x38, 0x42, 0x44, 0xff, 0x3f, 0x3e, 0x3c, 0xff, 0x3f, 0x40, 0x42, 0xff, 0x3d, 0x3e, 0x40, 0xff, 0x3e, 0x43, 0x46, 0xff, 0x3f, 0x3e, 0x3a, 0xff, 0x43, 0x40, 0x3b, 0xff, 
  0x48, 0x43, 0x3f, 0xff, 0x32, 0x3e, 0x3e, 0xff, 0x60, 0x3c, 0x26, 0xff, 0x5a, 0x36, 0x14, 0xff, 0x5a, 0x36, 0x1e, 0xff, 0x5e, 0x36, 0x1d, 0xff, 0x5d, 0x3b, 0x20, 0xff, 0x5f, 0x37, 0x1d, 0xff, 
  0x5c, 0x3a, 0x21, 0xff, 0x5e, 0x34, 0x1c, 0xff, 0x62, 0x37, 0x17, 0xff, 0x60, 0x3d, 0x1f, 0xff, 0x5d, 0x38, 0x25, 0xff, 0x5b, 0x38, 0x22, 0xff, 0x59, 0x35, 0x15, 0xff, 0x67, 0x3b, 0x20, 0xff, 
  0x62, 0x3d, 0x22, 0xff, 0x5a, 0x35, 0x18, 0xff, 0x5c, 0x33, 0x1d, 0xff, 0x5c, 0x3a, 0x1f, 0xff, 0x5e, 0x35, 0x21, 0xff, 0x61, 0x3a, 0x19, 0xff, 0x59, 0x34, 0x1a, 0xff, 0x5e, 0x3c, 0x21, 0xff, 
  0x5a, 0x35, 0x18, 0xff, 0x58, 0x3c, 0x26, 0xff, 0x61, 0x3a, 0x1d, 0xff, 0x5c, 0x38, 0x20, 0xff, 0x5f, 0x37, 0x1e, 0xff, 0x5c, 0x39, 0x23, 0xff, 0x40, 0x3e, 0x3f, 0xff, 0x43, 0x42, 0x3e, 0xff, 
  0x45, 0x3d, 0x3a, 0xff, 0x3c, 0x46, 0x48, 0xff, 0xe3, 0x6e, 0x21, 0xff, 0xe4, 0x76, 0x1f, 0xff, 0xe2, 0x74, 0x27, 0xff, 0xe0, 0x71, 0x20, 0xff, 0xe1, 0x73, 0x1e, 0xff, 0xde, 0x6f, 0x20, 0xff, 
  0xe4, 0x6f, 0x1f, 0xff, 0xe2, 0x72, 0x2a, 0xff, 0xe1, 0x71, 0x1f, 0xff, 0xe4, 0x6f, 0x1f, 0xff, 0xe2, 0x73, 0x21, 0xff, 0xe0, 0x70, 0x24, 0xff, 0xe0, 0x71, 0x1f, 0xff, 0xe0, 0x70, 0x24, 0xff, 
  0xe3, 0x6f, 0x1c, 0xff, 0xe3, 0x74, 0x25, 0xff, 0xe2, 0x72, 0x28, 0xff, 0xe4, 0x73, 0x23, 0xff, 0xe2, 0x72, 0x28, 0xff, 0xdf, 0x6f, 0x1d, 0xff, 0xde, 0x72, 0x29, 0xff, 0xde, 0x70, 0x21, 0xff, 
  0xe6, 0x72, 0x1f, 0xff, 0xde, 0x74, 0x28, 0xff, 0xe1, 0x6d, 0x18, 0xff, 0xe2, 0x74, 0x27, 0xff, 0xe7, 0x6e, 0x1f, 0xff, 0xdd, 0x6e, 0x26, 0xff, 0x40, 0x3e, 0x43, 0xff, 0x3d, 0x41, 0x42, 0xff, 
  0x45, 0x3d, 0x3a, 0xff, 0x37, 0x3e, 0x44, 0xff, 0xff, 0x67, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfb, 0x68, 0x01, 0xff, 0xfd, 0x69, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xff, 0x6a, 0x04, 0xff, 
  0xff, 0x6a, 0x00, 0xff, 0xf9, 0x67, 0x04, 0xff, 0xff, 0x6d, 0x05, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfc, 0x68, 0x00, 0xff, 0xff, 0x6a, 0x04, 0xff, 0xfc, 0x68, 0x00, 0xff, 0xfe, 0x69, 0x03, 0xff, 
  0xff, 0x69, 0x00, 0xff, 0xfe, 0x69, 0x02, 0xff, 0xfa, 0x68, 0x03, 0xff, 0xff, 0x67, 0x00, 0xff, 0xff, 0x6a, 0x03, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfe, 0x6c, 0x07, 0xff, 0xff, 0x6c, 0x02, 0xff, 
  0xff, 0x64, 0x00, 0xff, 0xff, 0x6e, 0x09, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xfd, 0x68, 0x01, 0xff, 0xff, 0x67, 0x00, 0xff, 0xff, 0x6a, 0x06, 0xff, 0x43, 0x40, 0x49, 0xff, 0x3c, 0x40, 0x43, 0xff, 
  0x46, 0x41, 0x3b, 0xff, 0x3a, 0x43, 0x4a, 0xff, 0xff, 0x65, 0x00, 0xff, 0xfc, 0x69, 0x01, 0xff, 0xfd, 0x6a, 0x02, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfb, 0x67, 0x03, 0xff, 
  0xfb, 0x66, 0x00, 0xff, 0xfd, 0x6b, 0x06, 0xff, 0xfa, 0x65, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xfd, 0x69, 0x05, 0xff, 0xfe, 0x6c, 0x05, 0xff, 0xff, 0x6c, 0x03, 0xff, 0xff, 0x69, 0x00, 0xff, 
  0xff, 0x68, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6f, 0x0b, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfb, 0x68, 0x00, 0xff, 0xfe, 0x6c, 0x00, 0xff, 0xfb, 0x67, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 
  0xff, 0x6a, 0x04, 0xff, 0xf9, 0x64, 0x00, 0xff, 0xff, 0x6d, 0x08, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfc, 0x68, 0x00, 0xff, 0xff, 0x6c, 0x00, 0xff, 0x3e, 0x3d, 0x42, 0xff, 0x40, 0x3e, 0x3f, 0xff, 
  0x3d, 0x3c, 0x38, 0xff, 0x3f, 0x44, 0x4a, 0xff, 0xff, 0x67, 0x02, 0xff, 0xff, 0x6a, 0x03, 0xff, 0xff, 0x69, 0x01, 0xff, 0xfd, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x6c, 0x08, 0xff, 
  0xfc, 0x6a, 0x03, 0xff, 0xff, 0x6a, 0x01, 0xff, 0xf0, 0x6e, 0x18, 0xff, 0xf2, 0x73, 0x16, 0xff, 0xec, 0x6a, 0x16, 0xff, 0xfb, 0x6a, 0x01, 0xff, 0xf0, 0x6b, 0x0e, 0xff, 0xf3, 0x6d, 0x0e, 0xff, 
  0xfd, 0x6a, 0x03, 0xff, 0xff, 0x67, 0x00, 0xff, 0xfd, 0x6b, 0x04, 0xff, 0xf0, 0x6e, 0x0e, 0xff, 0xff, 0x6b, 0x01, 0xff, 0xf0, 0x71, 0x14, 0xff, 0xff, 0x68, 0x00, 0xff, 0xf2, 0x6f, 0x14, 0xff, 
  0xee, 0x6b, 0x11, 0xff, 0xe2, 0x70, 0x27, 0xff, 0xfa, 0x68, 0x03, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xfe, 0x6b, 0x01, 0xff, 0xfb, 0x67, 0x00, 0xff, 0x41, 0x42, 0x46, 0xff, 0x45, 0x41, 0x40, 0xff, 
  0x39, 0x42, 0x3f, 0xff, 0x44, 0x40, 0x3f, 0xff, 0xfa, 0x67, 0x00, 0xff, 0xfe, 0x65, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfc, 0x6c, 0x00, 0xff, 0xfc, 0x69, 0x01, 0xff, 0xfb, 0x6a, 0x01, 0xff, 
  0xf8, 0x6b, 0x01, 0xff, 0xff, 0x66, 0x00, 0xff, 0x86, 0x31, 0x00, 0xff, 0x7e, 0x2b, 0x00, 0xff, 0x8a, 0x2c, 0x00, 0xff, 0xff, 0x6c, 0x02, 0xff, 0x8b, 0x2b, 0x00, 0xff, 0x8b, 0x2f, 0x00, 0xff, 
  0xf7, 0x6b, 0x0a, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0x8c, 0x30, 0x00, 0xff, 0xf9, 0x65, 0x00, 0xff, 0x86, 0x2f, 0x00, 0xff, 0xff, 0x65, 0x00, 0xff, 0x86, 0x29, 0x00, 0xff, 
  0x86, 0x2d, 0x00, 0xff, 0x61, 0x3b, 0x26, 0xff, 0xff, 0x6b, 0x01, 0xff, 0xfa, 0x65, 0x00, 0xff, 0xff, 0x6f, 0x03, 0xff, 0xfb, 0x6b, 0x00, 0xff, 0x3a, 0x42, 0x45, 0xff, 0x3e, 0x3d, 0x38, 0xff, 
  0x3b, 0x44, 0x43, 0xff, 0x44, 0x3f, 0x3c, 0xff, 0xfd, 0x6b, 0x04, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xf9, 0x6b, 0x00, 0xff, 0xfd, 0x6b, 0x04, 0xff, 0xfc, 0x6a, 0x00, 0xff, 
  0xf8, 0x6b, 0x01, 0xff, 0xff, 0x68, 0x00, 0xff, 0xcc, 0x74, 0x34, 0xff, 0xce, 0x79, 0x38, 0xff, 0xd6, 0x77, 0x37, 0xff, 0xfd, 0x66, 0x00, 0xff, 0xd4, 0x77, 0x2a, 0xff, 0xcf, 0x75, 0x39, 0xff, 
  0xfb, 0x6c, 0x0c, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x66, 0x00, 0xff, 0xd1, 0x76, 0x2f, 0xff, 0xff, 0x69, 0x02, 0xff, 0xcf, 0x78, 0x35, 0xff, 0xff, 0x67, 0x00, 0xff, 0xd1, 0x77, 0x3b, 0xff, 
  0xd1, 0x75, 0x32, 0xff, 0x3f, 0x3e, 0x46, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x6d, 0x05, 0xff, 0xfc, 0x64, 0x00, 0xff, 0xfe, 0x6c, 0x00, 0xff, 0x3b, 0x43, 0x46, 0xff, 0x42, 0x3f, 0x3a, 0xff, 
  0x40, 0x41, 0x3c, 0xff, 0x3e, 0x40, 0x3b, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfc, 0x6b, 0x00, 0xff, 0xff, 0x6a, 0x03, 0xff, 0xfb, 0x6a, 0x01, 0xff, 0xfc, 0x6b, 0x02, 0xff, 0xff, 0x6a, 0x00, 0xff, 
  0xfc, 0x6b, 0x00, 0xff, 0xff, 0x67, 0x00, 0xff, 0xef, 0x6e, 0x10, 0xff, 0xee, 0x6f, 0x0e, 0xff, 0xf2, 0x6d, 0x12, 0xff, 0xff, 0x68, 0x00, 0xff, 0xef, 0x6f, 0x0c, 0xff, 0xec, 0x6e, 0x1a, 0xff, 
  0xff, 0x69, 0x02, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xeb, 0x70, 0x10, 0xff, 0xfd, 0x6a, 0x03, 0xff, 0xec, 0x70, 0x0e, 0xff, 0xff, 0x68, 0x00, 0xff, 0xee, 0x6e, 0x13, 0xff, 
  0xf0, 0x6b, 0x0e, 0xff, 0x43, 0x43, 0x4b, 0xff, 0xf6, 0x68, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x6d, 0x00, 0xff, 0xfc, 0x69, 0x00, 0xff, 0x3f, 0x3f, 0x47, 0xff, 0x42, 0x3f, 0x3a, 0xff, 
  0x47, 0x42, 0x3f, 0xff, 0x3c, 0x42, 0x3e, 0xff, 0xff, 0x68, 0x00, 0xff, 0xf8, 0x6b, 0x01, 0xff, 0xfe, 0x69, 0x03, 0xff, 0xff, 0x6b, 0x05, 0xff, 0xfc, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 
  0xfc, 0x6b, 0x02, 0xff, 0xff, 0x67, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xfc, 0x69, 0x09, 0xff, 
  0xff, 0x68, 0x00, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xf7, 0x6d, 0x02, 0xff, 0xfd, 0x6a, 0x03, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 
  0xff, 0x6c, 0x05, 0xff, 0x5c, 0x36, 0x21, 0xff, 0xfd, 0x6c, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfd, 0x65, 0x00, 0xff, 0xfe, 0x6b, 0x04, 0xff, 0x40, 0x3d, 0x44, 0xff, 0x48, 0x43, 0x40, 0xff, 
  0x44, 0x3e, 0x40, 0xff, 0x38, 0x43, 0x3f, 0xff, 0xff, 0x6a, 0x03, 0xff, 0xfa, 0x6c, 0x00, 0xff, 0xff, 0x69, 0x02, 0xff, 0xff, 0x68, 0x06, 0xff, 0xff, 0x6c, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 
  0xfa, 0x6b, 0x05, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xf9, 0x6c, 0x02, 0xff, 0xff, 0x67, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfe, 0x69, 0x02, 0xff, 
  0xfd, 0x6a, 0x02, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xf7, 0x6c, 0x03, 0xff, 0xfc, 0x6b, 0x02, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 
  0xf7, 0x69, 0x03, 0xff, 0xe8, 0x72, 0x1b, 0xff, 0xfe, 0x68, 0x00, 0xff, 0xff, 0x6a, 0x03, 0xff, 0xff, 0x6c, 0x07, 0xff, 0xfd, 0x6b, 0x08, 0xff, 0x40, 0x3e, 0x43, 0xff, 0x3f, 0x3e, 0x3c, 0xff, 
  0x44, 0x3d, 0x44, 0xff, 0x37, 0x40, 0x3b, 0xff, 0xff, 0x6a, 0x09, 0xff, 0xfd, 0x6c, 0x00, 0xff, 0xff, 0x69, 0x02, 0xff, 0xf3, 0x68, 0x15, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xfd, 0x68, 0x01, 0xff, 
  0xfa, 0x6a, 0x07, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfa, 0x6b, 0x03, 0xff, 0xff, 0x67, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfe, 0x69, 0x02, 0xff, 
  0xfe, 0x69, 0x02, 0xff, 0xfe, 0x69, 0x02, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xf9, 0x6b, 0x05, 0xff, 0xfe, 0x69, 0x02, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 
  0xf8, 0x6a, 0x04, 0xff, 0xff, 0x66, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xfb, 0x68, 0x01, 0xff, 0xfd, 0x68, 0x04, 0xff, 0xfb, 0x6c, 0x06, 0xff, 0x44, 0x3f, 0x43, 0xff, 0x40, 0x3e, 0x3f, 0xff, 
  0x42, 0x3f, 0x46, 0xff, 0x3f, 0x40, 0x3a, 0xff, 0xfc, 0x68, 0x08, 0xff, 0xfc, 0x6a, 0x00, 0xff, 0xfc, 0x69, 0x01, 0xff, 0xd3, 0x73, 0x39, 0xff, 0xff, 0x71, 0x03, 0xff, 0xfc, 0x69, 0x02, 0xff, 
  0xfa, 0x6b, 0x03, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfc, 0x6b, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 
  0xff, 0x68, 0x00, 0xff, 0xff, 0x69, 0x02, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfc, 0x6a, 0x07, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfe, 0x69, 0x02, 0xff, 
  0xff, 0x6e, 0x01, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfe, 0x69, 0x02, 0xff, 0xfe, 0x6c, 0x05, 0xff, 0xff, 0x6f, 0x04, 0xff, 0xfa, 0x69, 0x00, 0xff, 0x3f, 0x3e, 0x3c, 0xff, 0x42, 0x42, 0x44, 0xff, 
  0x40, 0x3f, 0x44, 0xff, 0x43, 0x42, 0x3e, 0xff, 0xfd, 0x69, 0x07, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xff, 0x70, 0x06, 0xff, 0x65, 0x2d, 0x0c, 0xff, 0xf8, 0x69, 0x00, 0xff, 0xfc, 0x67, 0x01, 0xff, 
  0xfd, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfc, 0x6b, 0x02, 0xff, 0xfe, 0x69, 0x02, 0xff, 0xfc, 0x6b, 0x00, 0xff, 0xfe, 0x69, 0x02, 0xff, 
  0xff, 0x67, 0x00, 0xff, 0xff, 0x69, 0x02, 0xff, 0xff, 0x68, 0x02, 0xff, 0xfd, 0x69, 0x05, 0xff, 0xff, 0x67, 0x00, 0xff, 0xfe, 0x69, 0x02, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 
  0xfd, 0x66, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfa, 0x67, 0x00, 0xff, 0xfd, 0x6c, 0x03, 0xff, 0xfb, 0x69, 0x00, 0xff, 0xff, 0x6e, 0x00, 0xff, 0x40, 0x40, 0x40, 0xff, 0x40, 0x40, 0x42, 0xff, 
  0x3e, 0x3d, 0x3b, 0xff, 0x41, 0x41, 0x43, 0xff, 0xfe, 0x6b, 0x03, 0xff, 0xfe, 0x68, 0x00, 0xff, 0xfc, 0x64, 0x00, 0xff, 0x52, 0x46, 0x38, 0xff, 0xfc, 0x69, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 
  0xfd, 0x6b, 0x00, 0xff, 0xff, 0x68, 0x02, 0xff, 0xfc, 0x6b, 0x02, 0xff, 0xfe, 0x6b, 0x00, 0xff, 0xfa, 0x6a, 0x09, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 
  0xfd, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfd, 0x69, 0x07, 0xff, 0xfc, 0x6b, 0x00, 0xff, 0xff, 0x67, 0x00, 0xff, 0xfc, 0x6b, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfc, 0x6b, 0x00, 0xff, 
  0xff, 0x6a, 0x02, 0xff, 0xfd, 0x6b, 0x06, 0xff, 0xff, 0x6c, 0x02, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x6d, 0x00, 0xff, 0xfa, 0x66, 0x00, 0xff, 0x3e, 0x42, 0x45, 0xff, 0x3f, 0x40, 0x3a, 0xff, 
  0x42, 0x41, 0x3c, 0xff, 0x41, 0x40, 0x46, 0xff, 0xfd, 0x69, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 0x39, 0x3f, 0x3d, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 
  0xfe, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6a, 0x03, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xfa, 0x6a, 0x0a, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 
  0xfd, 0x6a, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xfc, 0x6a, 0x07, 0xff, 0xfc, 0x6c, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfa, 0x6c, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 
  0xff, 0x68, 0x00, 0xff, 0xfb, 0x69, 0x06, 0xff, 0xfe, 0x6b, 0x01, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xff, 0x64, 0x00, 0xff, 0xfe, 0x6c, 0x00, 0xff, 0x40, 0x41, 0x43, 0xff, 0x41, 0x41, 0x3f, 0xff, 
  0x42, 0x3e, 0x3b, 0xff, 0x41, 0x41, 0x43, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x68, 0x01, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0x39, 0x3f, 0x3d, 0xff, 0xfe, 0x6e, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 
  0xfc, 0x6b, 0x02, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xf7, 0x6c, 0x07, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 
  0xfd, 0x6b, 0x00, 0xff, 0xf9, 0x6c, 0x02, 0xff, 0xfd, 0x6a, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xf9, 0x6c, 0x02, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 
  0xff, 0x68, 0x00, 0xff, 0xfc, 0x6a, 0x03, 0xff, 0xfc, 0x69, 0x00, 0xff, 0xfc, 0x6b, 0x02, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xf7, 0x6b, 0x00, 0xff, 0x45, 0x40, 0x3a, 0xff, 0x3c, 0x3d, 0x42, 0xff, 
  0x40, 0x40, 0x3e, 0xff, 0x43, 0x42, 0x47, 0xff, 0xff, 0x67, 0x00, 0xff, 0xfe, 0x69, 0x03, 0xff, 0xfe, 0x6b, 0x03, 0xff, 0x5f, 0x3d, 0x21, 0xff, 0xfa, 0x6a, 0x00, 0xff, 0xff, 0x6b, 0x04, 0xff, 
  0xfc, 0x6a, 0x03, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xf9, 0x6b, 0x05, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 
  0xff, 0x69, 0x00, 0xff, 0xfa, 0x6b, 0x05, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfa, 0x6b, 0x03, 0xff, 0xfd, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 
  0xff, 0x6b, 0x00, 0xff, 0xec, 0x6b, 0x10, 0xff, 0xff, 0x6b, 0x01, 0xff, 0xfe, 0x6b, 0x04, 0xff, 0xff, 0x65, 0x00, 0xff, 0xf8, 0x6e, 0x03, 0xff, 0x49, 0x42, 0x3c, 0xff, 0x43, 0x41, 0x4c, 0xff, 
  0x39, 0x3f, 0x3f, 0xff, 0x3e, 0x41, 0x46, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xdd, 0x6e, 0x1d, 0xff, 0xfe, 0x6c, 0x00, 0xff, 0xfe, 0x66, 0x01, 0xff, 
  0xfd, 0x6a, 0x02, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x69, 0x02, 0xff, 0xfc, 0x6a, 0x07, 0xff, 0xff, 0x67, 0x00, 0xff, 0xff, 0x69, 0x02, 0xff, 0xfd, 0x69, 0x05, 0xff, 
  0xff, 0x67, 0x00, 0xff, 0xfe, 0x69, 0x05, 0xff, 0xff, 0x68, 0x03, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfa, 0x6b, 0x05, 0xff, 0xfc, 0x6a, 0x03, 0xff, 0xff, 0x68, 0x00, 0xff, 
  0xf9, 0x68, 0x00, 0xff, 0x91, 0x32, 0x00, 0xff, 0xfc, 0x68, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x6a, 0x05, 0xff, 0xf7, 0x68, 0x00, 0xff, 0x42, 0x3e, 0x3b, 0xff, 0x42, 0x3d, 0x43, 0xff, 
  0x40, 0x45, 0x49, 0xff, 0x3f, 0x40, 0x45, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xfd, 0x66, 0x00, 0xff, 0xe3, 0x72, 0x22, 0xff, 0xfe, 0x68, 0x00, 0xff, 0xff, 0x6b, 0x07, 0xff, 
  0xfd, 0x6a, 0x02, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfe, 0x69, 0x03, 0xff, 0xfd, 0x69, 0x05, 0xff, 0xff, 0x67, 0x00, 0xff, 0xff, 0x68, 0x02, 0xff, 0xfe, 0x68, 0x07, 0xff, 
  0xff, 0x67, 0x00, 0xff, 0xff, 0x67, 0x03, 0xff, 0xff, 0x68, 0x05, 0xff, 0xff, 0x67, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfd, 0x6a, 0x03, 0xff, 0xfa, 0x6b, 0x05, 0xff, 0xff, 0x69, 0x00, 0xff, 
  0xfb, 0x6a, 0x00, 0xff, 0xb6, 0x7a, 0x56, 0xff, 0xff, 0x6d, 0x02, 0xff, 0xff, 0x66, 0x00, 0xff, 0xff, 0x67, 0x06, 0xff, 0xfc, 0x69, 0x02, 0xff, 0x43, 0x43, 0x43, 0xff, 0x41, 0x3c, 0x40, 0xff, 
  0x3b, 0x39, 0x44, 0xff, 0x40, 0x3f, 0x3d, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x69, 0x02, 0xff, 0xff, 0x6c, 0x08, 0xff, 0x5c, 0x37, 0x1d, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xf9, 0x6a, 0x02, 0xff, 
  0xfc, 0x6b, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x69, 0x05, 0xff, 
  0xfe, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfd, 0x6a, 0x03, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 0xfd, 0x6a, 0x00, 0xff, 0xfa, 0x6b, 0x03, 0xff, 0xff, 0x69, 0x00, 0xff, 
  0xff, 0x6c, 0x00, 0xff, 0x4c, 0x38, 0x31, 0xff, 0xfd, 0x65, 0x00, 0xff, 0xff, 0x68, 0x01, 0xff, 0xff, 0x67, 0x07, 0xff, 0xfe, 0x6b, 0x01, 0xff, 0x39, 0x3f, 0x3f, 0xff, 0x3d, 0x41, 0x40, 0xff, 
  0x44, 0x41, 0x4a, 0xff, 0x43, 0x3f, 0x40, 0xff, 0xfc, 0x69, 0x01, 0xff, 0xff, 0x6b, 0x06, 0xff, 0xfe, 0x68, 0x07, 0xff, 0x53, 0x3c, 0x34, 0xff, 0xff, 0x66, 0x00, 0xff, 0xf8, 0x69, 0x01, 0xff, 
  0xfc, 0x6b, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xfc, 0x6b, 0x02, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 
  0xfd, 0x6b, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfd, 0x6a, 0x03, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfc, 0x6a, 0x03, 0xff, 0xff, 0x69, 0x00, 0xff, 
  0xfd, 0x67, 0x00, 0xff, 0x43, 0x40, 0x47, 0xff, 0xff, 0x6f, 0x01, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x67, 0x05, 0xff, 0xfd, 0x68, 0x01, 0xff, 0x40, 0x44, 0x43, 0xff, 0x3f, 0x43, 0x42, 0xff, 
  0x3d, 0x3d, 0x3f, 0xff, 0x43, 0x41, 0x46, 0xff, 0xfb, 0x6e, 0x02, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfc, 0x65, 0x00, 0xff, 0x72, 0x36, 0x12, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x6c, 0x08, 0xff, 
  0xfd, 0x6a, 0x02, 0xff, 0xfd, 0x6a, 0x02, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 
  0xfc, 0x6c, 0x00, 0xff, 0xfe, 0x6b, 0x00, 0xff, 0xfd, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 
  0xff, 0x69, 0x00, 0xff, 0x41, 0x3f, 0x42, 0xff, 0xf7, 0x6a, 0x00, 0xff, 0xfb, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x03, 0xff, 0xfc, 0x6b, 0x02, 0xff, 0x44, 0x3f, 0x39, 0xff, 0x3f, 0x3e, 0x3c, 0xff, 
  0x41, 0x42, 0x3d, 0xff, 0x3f, 0x40, 0x44, 0xff, 0xf6, 0x6c, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xb5, 0x7b, 0x56, 0xff, 0xfd, 0x6b, 0x00, 0xff, 0xff, 0x68, 0x03, 0xff, 
  0xfe, 0x69, 0x02, 0xff, 0xfe, 0x69, 0x03, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfc, 0x6b, 0x00, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 
  0xfc, 0x6b, 0x00, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfe, 0x69, 0x02, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 
  0xff, 0x6b, 0x00, 0xff, 0x50, 0x3c, 0x31, 0xff, 0xf8, 0x6c, 0x00, 0xff, 0xfc, 0x6e, 0x02, 0xff, 0xff, 0x65, 0x00, 0xff, 0xfd, 0x6c, 0x03, 0xff, 0x43, 0x3e, 0x3a, 0xff, 0x44, 0x40, 0x3f, 0xff, 
  0x43, 0x3f, 0x34, 0xff, 0x3e, 0x43, 0x3d, 0xff, 0xf8, 0x6d, 0x04, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 0x4d, 0x3d, 0x2e, 0xff, 0xfb, 0x67, 0x00, 0xff, 0xff, 0x6d, 0x03, 0xff, 
  0xff, 0x68, 0x00, 0xff, 0xff, 0x6b, 0x04, 0xff, 0xfc, 0x67, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xf9, 0x6b, 0x05, 0xff, 0xfb, 0x68, 0x00, 0xff, 0xfe, 0x6d, 0x02, 0xff, 
  0xfc, 0x69, 0x00, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x69, 0x01, 0xff, 0xf9, 0x67, 0x04, 0xff, 0xfd, 0x6b, 0x04, 0xff, 0xfd, 0x69, 0x00, 0xff, 0xff, 0x6c, 0x00, 0xff, 0xff, 0x6b, 0x00, 0xff, 
  0xff, 0x69, 0x00, 0xff, 0x6e, 0x34, 0x0f, 0xff, 0xf7, 0x6d, 0x00, 0xff, 0xfc, 0x6e, 0x02, 0xff, 0xff, 0x66, 0x00, 0xff, 0xfe, 0x6b, 0x01, 0xff, 0x39, 0x3f, 0x3d, 0xff, 0x41, 0x42, 0x3d, 0xff, 
  0x45, 0x41, 0x38, 0xff, 0x3a, 0x3f, 0x3b, 0xff, 0xfa, 0x6b, 0x03, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x67, 0x00, 0xff, 0x50, 0x43, 0x32, 0xff, 0xed, 0x6e, 0x11, 0xff, 0xfd, 0x66, 0x00, 0xff, 
  0xf5, 0x6e, 0x12, 0xff, 0xed, 0x6b, 0x13, 0xff, 0xff, 0x6a, 0x03, 0xff, 0xf3, 0x6e, 0x11, 0xff, 0xfa, 0x66, 0x00, 0xff, 0xfe, 0x6c, 0x05, 0xff, 0xfe, 0x6a, 0x00, 0xff, 0xfd, 0x6a, 0x02, 0xff, 
  0xfd, 0x69, 0x00, 0xff, 0xfc, 0x6a, 0x03, 0xff, 0xff, 0x67, 0x01, 0xff, 0xfd, 0x69, 0x07, 0xff, 0xff, 0x6d, 0x06, 0xff, 0xf3, 0x6e, 0x0f, 0xff, 0xef, 0x6d, 0x0b, 0xff, 0xee, 0x6d, 0x0f, 0xff, 
  0xf3, 0x70, 0x18, 0xff, 0xd1, 0x73, 0x35, 0xff, 0xf9, 0x6d, 0x00, 0xff, 0xfa, 0x6b, 0x03, 0xff, 0xff, 0x63, 0x00, 0xff, 0xff, 0x6e, 0x02, 0xff, 0x3d, 0x43, 0x43, 0xff, 0x41, 0x41, 0x41, 0xff, 
  0x3f, 0x41, 0x3e, 0xff, 0x40, 0x41, 0x46, 0xff, 0xfb, 0x68, 0x00, 0xff, 0xff, 0x68, 0x02, 0xff, 0xfc, 0x6e, 0x02, 0xff, 0x67, 0x31, 0x05, 0xff, 0x85, 0x31, 0x00, 0xff, 0xff, 0x6c, 0x03, 0xff, 
  0x89, 0x2b, 0x00, 0xff, 0x88, 0x2f, 0x00, 0xff, 0xf9, 0x65, 0x00, 0xff, 0x88, 0x2e, 0x00, 0xff, 0xf9, 0x70, 0x10, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x67, 0x00, 0xff, 0xfa, 0x67, 0x00, 0xff, 
  0xff, 0x6a, 0x00, 0xff, 0xf6, 0x6d, 0x0d, 0xff, 0xfc, 0x6a, 0x05, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xf9, 0x66, 0x00, 0xff, 0x89, 0x24, 0x00, 0xff, 0x8c, 0x34, 0x00, 0xff, 0x86, 0x2a, 0x00, 0xff, 
  0x85, 0x27, 0x00, 0xff, 0xf4, 0x73, 0x15, 0xff, 0xfe, 0x6c, 0x00, 0xff, 0xfd, 0x6b, 0x06, 0xff, 0xff, 0x69, 0x09, 0xff, 0xfe, 0x63, 0x00, 0xff, 0x45, 0x44, 0x42, 0xff, 0x3a, 0x3d, 0x42, 0xff, 
  0x3d, 0x3f, 0x3e, 0xff, 0x40, 0x40, 0x4a, 0xff, 0xff, 0x6d, 0x00, 0xff, 0xff, 0x67, 0x03, 0xff, 0xf8, 0x69, 0x01, 0xff, 0xd3, 0x76, 0x30, 0xff, 0xcf, 0x7b, 0x35, 0xff, 0xfd, 0x64, 0x00, 0xff, 
  0xd6, 0x77, 0x33, 0xff, 0xc9, 0x73, 0x38, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xce, 0x74, 0x38, 0xff, 0xf5, 0x6c, 0x0c, 0xff, 0xff, 0x66, 0x00, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x6a, 0x03, 0xff, 
  0xff, 0x6b, 0x00, 0xff, 0xf4, 0x6b, 0x0b, 0xff, 0xfb, 0x69, 0x04, 0xff, 0xff, 0x68, 0x00, 0xff, 0xfd, 0x69, 0x00, 0xff, 0xe2, 0x7c, 0x34, 0xff, 0xca, 0x71, 0x35, 0xff, 0xd3, 0x75, 0x37, 0xff, 
  0xd5, 0x79, 0x38, 0xff, 0xfc, 0x68, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xfd, 0x6b, 0x08, 0xff, 0xff, 0x69, 0x0b, 0xff, 0xff, 0x67, 0x00, 0xff, 0x41, 0x40, 0x3e, 0xff, 0x41, 0x40, 0x46, 0xff, 
  0x42, 0x41, 0x3f, 0xff, 0x39, 0x40, 0x46, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xfe, 0x65, 0x00, 0xff, 0xff, 0x6b, 0x04, 0xff, 0xef, 0x6d, 0x0b, 0xff, 0xf1, 0x6e, 0x13, 0xff, 0xff, 0x6b, 0x07, 0xff, 
  0xee, 0x6e, 0x0d, 0xff, 0xea, 0x71, 0x18, 0xff, 0xff, 0x6b, 0x00, 0xff, 0xef, 0x6f, 0x14, 0xff, 0xff, 0x6b, 0x04, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x66, 0x00, 0xff, 0xfd, 0x66, 0x00, 0xff, 
  0xfd, 0x69, 0x00, 0xff, 0xf9, 0x6a, 0x01, 0xff, 0xff, 0x6a, 0x00, 0xff, 0xff, 0x68, 0x00, 0xff, 0xff, 0x69, 0x00, 0xff, 0xf2, 0x6d, 0x10, 0xff, 0xed, 0x6b, 0x13, 0xff, 0xf4, 0x6e, 0x17, 0xff, 
  0xf0, 0x6f, 0x13, 0xff, 0xff, 0x69, 0x00, 0xff, 0xff, 0x6c, 0x00, 0xff, 0xfc, 0x6d, 0x07, 0xff, 0xfd, 0x65, 0x02, 0xff, 0xff, 0x6b, 0x00, 0xff, 0x3d, 0x41, 0x40, 0xff, 0x43, 0x3f, 0x3e, 0xff, 
  0x41, 0x40, 0x3e, 0xff, 0x39, 0x41, 0x43, 0xff, 0xe5, 0x6e, 0x1a, 0xff, 0xe3, 0x72, 0x24, 0xff, 0xe1, 0x70, 0x24, 0xff, 0xe4, 0x76, 0x21, 0xff, 0xde, 0x6d, 0x1f, 0xff, 0xe2, 0x6e, 0x23, 0xff, 
  0xe2, 0x73, 0x22, 0xff, 0xda, 0x70, 0x24, 0xff, 0xe4, 0x6e, 0x17, 0xff, 0xe3, 0x74, 0x25, 0xff, 0xe4, 0x6f, 0x1f, 0xff, 0xe5, 0x6e, 0x1e, 0xff, 0xe3, 0x73, 0x29, 0xff, 0xe4, 0x73, 0x25, 0xff, 
  0xde, 0x71, 0x1f, 0xff, 0xe4, 0x73, 0x23, 0xff, 0xe5, 0x72, 0x22, 0xff, 0xe3, 0x70, 0x1f, 0xff, 0xe3, 0x72, 0x22, 0xff, 0xe1, 0x72, 0x23, 0xff, 0xe2, 0x71, 0x23, 0xff, 0xe1, 0x6d, 0x22, 0xff, 
  0xe0, 0x71, 0x22, 0xff, 0xe5, 0x72, 0x21, 0xff, 0xe2, 0x72, 0x20, 0xff, 0xd9, 0x6e, 0x20, 0xff, 0xe7, 0x73, 0x26, 0xff, 0xe3, 0x75, 0x1e, 0xff, 0x39, 0x42, 0x41, 0xff, 0x46, 0x40, 0x40, 0xff, 
  0x3f, 0x3f, 0x3d, 0xff, 0x3e, 0x42, 0x41, 0xff, 0x5c, 0x38, 0x18, 0xff, 0x5d, 0x39, 0x23, 0xff, 0x60, 0x38, 0x1f, 0xff, 0x5e, 0x39, 0x1c, 0xff, 0x57, 0x39, 0x1d, 0xff, 0x5f, 0x3a, 0x1d, 0xff, 
  0x5f, 0x3c, 0x20, 0xff, 0x58, 0x38, 0x21, 0xff, 0x60, 0x37, 0x19, 0xff, 0x5e, 0x39, 0x1c, 0xff, 0x5d, 0x38, 0x1b, 0xff, 0x5b, 0x34, 0x15, 0xff, 0x5c, 0x3e, 0x26, 0xff, 0x5a, 0x36, 0x1c, 0xff, 
  0x5d, 0x39, 0x21, 0xff, 0x60, 0x39, 0x1c, 0xff, 0x5c, 0x38, 0x1e, 0xff, 0x5c, 0x35, 0x18, 0xff, 0x58, 0x38, 0x1f, 0xff, 0x5e, 0x3a, 0x1a, 0xff, 0x5c, 0x39, 0x1d, 0xff, 0x61, 0x39, 0x1f, 0xff, 
  0x5d, 0x39, 0x1f, 0xff, 0x5f, 0x37, 0x1e, 0xff, 0x59, 0x3a, 0x1e, 0xff, 0x5f, 0x3a, 0x1d, 0xff, 0x60, 0x39, 0x1c, 0xff, 0x5b, 0x3a, 0x19, 0xff, 0x3a, 0x40, 0x3c, 0xff, 0x40, 0x3b, 0x3f, 0xff, 
  0x3e, 0x40, 0x3f, 0xff, 0x3e, 0x40, 0x3d, 0xff, 0x41, 0x43, 0x3e, 0xff, 0x3d, 0x40, 0x45, 0xff, 0x3f, 0x3d, 0x40, 0xff, 0x3f, 0x3f, 0x3d, 0xff, 0x3c, 0x46, 0x45, 0xff, 0x3f, 0x40, 0x3b, 0xff, 
  0x3d, 0x3d, 0x3d, 0xff, 0x40, 0x44, 0x47, 0xff, 0x42, 0x3e, 0x3b, 0xff, 0x40, 0x3f, 0x3b, 0xff, 0x40, 0x40, 0x3e, 0xff, 0x42, 0x41, 0x3d, 0xff, 0x38, 0x42, 0x44, 0xff, 0x3d, 0x41, 0x40, 0xff, 
  0x40, 0x41, 0x45, 0xff, 0x43, 0x3f, 0x3e, 0xff, 0x3e, 0x3f, 0x43, 0xff, 0x41, 0x40, 0x3c, 0xff, 0x3b, 0x43, 0x45, 0xff, 0x3f, 0x40, 0x3a, 0xff, 0x3d, 0x3f, 0x3c, 0xff, 0x42, 0x41, 0x3f, 0xff, 
  0x3f, 0x3f, 0x41, 0xff, 0x41, 0x3f, 0x42, 0xff, 0x3b, 0x43, 0x45, 0xff, 0x3f, 0x3e, 0x39, 0xff, 0x42, 0x3f, 0x3a, 0xff, 0x3d, 0x43, 0x3f, 0xff, 0x3d, 0x43, 0x3f, 0xff, 0x42, 0x3f, 0x46, 0xff};

// clang-format on

inline std::vector<unsigned char>
ReadBlobUnsafe(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open())
        return {};

    std::ifstream::pos_type pos = file.tellg();

    // What happens if the OS supports really big files.
    // It may be larger than 32 bits?
    // This will silently truncate the value/
    std::streamoff length = pos;

    std::vector<unsigned char> bytes(length);
    if (pos > 0)
        { // Manuall memory management.
            // Not a good idea use a container/.
            file.seekg(0, std::ios::beg);
            file.read((char*)bytes.data(), length);
        }
    file.close();
    return bytes;
};

int
main()
{

    Fox::DContextConfig config;
    config.logOutputFunction = &Log;
    config.warningFunction   = &Log;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);

    {
        if (!glfwInit())
            {
                throw std::runtime_error("Failed to glfwInit");
            }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // as Vulkan, there is no need to create a context
        GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
        if (!window)
            {
                throw std::runtime_error("Failed to glfwCreateWindow");
            }
        Fox::WindowData windowData;
        windowData._hwnd     = glfwGetWin32Window(window);
        windowData._instance = GetModuleHandle(NULL);

        Fox::DSwapchain swapchain;
        auto            presentMode = Fox::EPresentMode::IMMEDIATE_KHR;
        auto            format      = Fox::EFormat::B8G8R8A8_UNORM;
        if (!context->CreateSwapchain(&windowData, presentMode, format, &swapchain))
            {
                throw std::runtime_error("Failed to CreateSwapchain");
            }

        // Create vertex layout
        Fox::VertexLayoutInfo   position("SV_POSITION", Fox::EFormat::R32G32B32_FLOAT, 0, Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexLayoutInfo   color("Color0", Fox::EFormat::R32G32B32A32_FLOAT, 3 * sizeof(float), Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::DVertexInputLayout vertexLayout = context->CreateVertexLayout({ position, color });
        constexpr uint32_t      stride       = 7 * sizeof(float);

        Fox::ShaderSource shaderSource;
        shaderSource.VertexLayout            = vertexLayout;
        shaderSource.VertexStride            = stride;
        shaderSource.SourceCode.VertexShader = ReadBlobUnsafe("vertex.spv");
        shaderSource.SourceCode.PixelShader  = ReadBlobUnsafe("fragment.spv");

        shaderSource.SetsLayout.SetsLayout[0].insert({ 0, Fox::ShaderDescriptorBindings("Camera", Fox::EBindingType::UNIFORM_BUFFER_OBJECT, sizeof(float) * 16, 1, Fox::EShaderStage::VERTEX) });
        shaderSource.SetsLayout.SetsLayout[1].insert({ 0, Fox::ShaderDescriptorBindings("MatrixUbo", Fox::EBindingType::UNIFORM_BUFFER_OBJECT, sizeof(float) * 16, 1, Fox::EShaderStage::VERTEX) });
        shaderSource.SetsLayout.SetsLayout[2].insert({ 0, Fox::ShaderDescriptorBindings("MyTexture", Fox::EBindingType::SAMPLER, 1, 1, Fox::EShaderStage::FRAGMENT) });

        shaderSource.ColorAttachments.push_back({ format });
        shaderSource.DepthStencilAttachment = Fox::EFormat::DEPTH32_FLOAT;

        Fox::DShader shader = context->CreateShader(shaderSource);

        // clang-format off
        constexpr std::array<float, 84> ndcQuad{            
            -1, -1, -1,//pos
            0, 1, 0, 1,// color
            1, -1, -1,//pos
            0, 0, 1, 1,// color
            1, 1, -1,//pos
            0, 1, 1, 1,// color
            //----------------//
            -1, -1, -1,//pos
            0, 1, 0, 1,// color
            1, 1, -1,//pos
            0, 0, 1, 1,// color
            -1, 1, -1,//pos
            0, 1, 1, 1,// color
            //----------------//
            -1, -1, 1,//pos
            0, 1, 0, 1,// color
            1, -1, 1,//pos
            0, 0, 1, 1,// color
            1, 1, 1,//pos
            0, 1, 1, 1,// color
            //----------------//
            -1, -1, 1,//pos
            0, 1, 0, 1,// color
            1, 1, 1,//pos
            0, 0, 1, 1,// color
            -1, 1, 1,//pos
            0, 1, 1, 1,// color
        };
        // clang-format on
        constexpr size_t bufSize = sizeof(float) * ndcQuad.size();
        Fox::DBuffer     quad    = context->CreateVertexBuffer(bufSize);

        Fox::CopyDataCommand copy;
        copy.CopyVertex(quad, 0, (void*)ndcQuad.data(), bufSize);
        context->SubmitCopy(std::move(copy));

        Fox::DImage       depthBuffer  = context->CreateImage(Fox::EFormat::DEPTH32_FLOAT, 640, 480, 1);
        Fox::DFramebuffer swapchainFbo = context->CreateSwapchainFramebuffer(swapchain, depthBuffer);

        Fox::DRenderPassAttachment  colorAttachment(format,
        Fox::ESampleBit::COUNT_1_BIT,
        Fox::ERenderPassLoad::Clear,
        Fox::ERenderPassStore::Store,
        Fox::ERenderPassLayout::Undefined,
        Fox::ERenderPassLayout::Present,
        Fox::EAttachmentReference::COLOR_ATTACHMENT);
        Fox::DRenderPassAttachments renderPass;
        renderPass.Attachments.push_back(colorAttachment);

        Fox::DRenderPassAttachment depthAttachment(Fox::EFormat::DEPTH32_FLOAT,
        Fox::ESampleBit::COUNT_1_BIT,
        Fox::ERenderPassLoad::Clear,
        Fox::ERenderPassStore::Store,
        Fox::ERenderPassLayout::Undefined,
        Fox::ERenderPassLayout::Present,
        Fox::EAttachmentReference::DEPTH_STENCIL_ATTACHMENT);
        renderPass.Attachments.push_back(depthAttachment);

        Fox::DBuffer transformUniformBuffer = context->CreateUniformBuffer(sizeof(float) * 16);
        float        yaw{ 0 };

        Fox::DImage texture = context->CreateImage(Fox::EFormat::R8G8B8A8_UNORM, imageWidth, imageHeight, 1);
        {
            Fox::CopyDataCommand copy;
            copy.CopyImageMipMap(texture, 0, (void*)image, imageWidth, imageHeight, 0, sizeof(image));
            context->SubmitCopy(std::move(copy));
        }

        const auto   perspective         = glm::perspective(glm::radians(60.f), 1.f, 0.f, 1.f);
        const auto   view                = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -5));
        glm::mat4    cameraProjection    = perspective * view;
        Fox::DBuffer cameraUniformBuffer = context->CreateUniformBuffer(sizeof(float) * 16);
        {
            Fox::CopyDataCommand copy;
            copy.CopyUniformBuffer(cameraUniformBuffer, glm::value_ptr(cameraProjection), sizeof(float) * 16);
            context->SubmitCopy(std::move(copy));
        }

        while (!glfwWindowShouldClose(window))
            {
                // Keep running
                glfwPollEvents();

                int w, h;
                glfwGetWindowSize(window, &w, &h);
                Fox::DViewport viewport{ 0, 0, w, h, 0.f, 1.f };

                {
                    yaw += 0.001f;
                    const float sinA = std::sin(yaw);
                    const float cosA = std::cos(yaw);

                    float                matrix[4][4]{ { cosA, 0, -sinA, 0 }, { 0, 1, 0, 0 }, { sinA, 0, cosA, 0 }, { 0, 0, 0, 1 } };
                    Fox::CopyDataCommand copy;
                    copy.CopyUniformBuffer(transformUniformBuffer, &matrix[0][0], sizeof(float) * 16);
                    context->SubmitCopy(std::move(copy));
                }

                Fox::RenderPassData draw(swapchainFbo, viewport, renderPass);
                draw.ClearColor(1, 0, 0, 1);
                draw.ClearDepthStencil(0, 0);

                {
                    Fox::DrawCommand drawTriangle(shader);
                    drawTriangle.BindBufferUniformBuffer(0, 0, cameraUniformBuffer);
                    drawTriangle.BindBufferUniformBuffer(1, 0, transformUniformBuffer);
                    drawTriangle.BindImageArray(2, 0, { texture });
                    drawTriangle.Draw(quad, 0, 12);
                    draw.AddDrawCommand(std::move(drawTriangle));
                }

                context->SubmitPass(std::move(draw));

                context->AdvanceFrame();
            }

        context->DestroyShader(shader);
        context->DestroyImage(texture);
        context->DestroyImage(depthBuffer);
        context->DestroyVertexBuffer(quad);
        context->DestroyUniformBuffer(transformUniformBuffer);
        context->DestroyUniformBuffer(cameraUniformBuffer);
        context->DestroyFramebuffer(swapchainFbo);
        context->DestroySwapchain(swapchain);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    delete context;

    return 0;
}