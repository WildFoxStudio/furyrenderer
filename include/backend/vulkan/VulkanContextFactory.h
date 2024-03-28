// Copyright RedFox Studio 2022

#pragma once

#include "IContext.h"

#ifdef FURY_EXPORT
#define FURY_API __declspec(dllexport)
#else
#define FURY_API __declspec(dllimport)
#endif

namespace Fox
{
IContext* CreateVulkanContext(DContextConfig const* const config);
}