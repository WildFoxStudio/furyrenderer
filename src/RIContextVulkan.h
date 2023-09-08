// Copyright RedFox Studio 2022

#pragma once

#if defined(FOX_USE_VULKAN)

#include "RenderInterface/vulkan/RIContextVk.h"

namespace Fox
{
struct RIContextTypeVulkan
{
    struct context
    {
        using type = RIContextVk;
    };
};

}

#endif