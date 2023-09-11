// Copyright RedFox Studio 2022

#include "backend/vulkan/VulkanContextFactory.h"

#include "VulkanContext.h"

namespace Fox
{

IContext*
CreateVulkanContext(DContextConfig const* const config)
{
    return new VulkanContext(config);
}

}