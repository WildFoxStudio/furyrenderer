#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk.h>

#define VKFAILED(result) ((result != VK_SUCCESS ? true : false))
#define VKSUCCEDED(result) ((result == VK_SUCCESS ? true : false))

inline const char*
VkErrorString(enum VkResult result)
{
    switch (result)
        {
            case VK_SUCCESS:
                return "VK_SUCCESS";
                break;
            case VK_NOT_READY:
                return "VK_NOT_READY";
                break;
            case VK_TIMEOUT:
                return "VK_TIMEOUT";
                break;
            case VK_EVENT_SET:
                return "VK_EVENT_SET";
                break;
            case VK_EVENT_RESET:
                return "VK_EVENT_RESET";
                break;
            case VK_INCOMPLETE:
                return "VK_INCOMPLETE";
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                return "VK_ERROR_INITIALIZATION_FAILED";
                break;
            case VK_ERROR_DEVICE_LOST:
                return "VK_ERROR_DEVICE_LOST";
                break;
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "VK_ERROR_MEMORY_MAP_FAILED";
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "VK_ERROR_LAYER_NOT_PRESENT";
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
                break;
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "VK_ERROR_FEATURE_NOT_PRESENT";
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "VK_ERROR_TOO_MANY_OBJECTS";
                break;
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                break;
            case VK_ERROR_FRAGMENTED_POOL:
                return "VK_ERROR_FRAGMENTED_POOL";
                break;
            case VK_ERROR_UNKNOWN:
                return "VK_ERROR_UNKNOWN";
                break;
            default:
                return "UNKNOWN VK ERROR";
                break;
        }
}
