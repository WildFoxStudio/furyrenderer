#include "WindowFixture.h"

#include "backend/vulkan/VulkanContextFactory.h"

TEST_F(WindowFixture, ShouldCreateAndDestroyContext)
{

    Fox::DContextConfig config;
    config.warningFunction = &WarningAssert;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);
    ASSERT_NE(context, nullptr);

    delete context;
}
