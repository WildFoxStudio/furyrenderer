// Copyright RedFox Studio 2022

#include <iostream>

#include "IContext.h"
#include "backend/vulkan/VulkanContextFactory.h"

#include <iostream>

void
Log(const char* msg)
{
    std::cout << msg << std::endl;
};

int
main()
{

    Fox::DContextConfig config;
    config.logOutputFunction = &Log;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);

    delete context;
    return 0;
}