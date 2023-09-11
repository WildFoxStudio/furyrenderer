// Copyright RedFox Studio 2022

#pragma once

#include "IContext.h"

namespace Fox
{
IContext* CreateVulkanContext(DContextConfig const*const config);
}