#include "WindowFixture.h"

#include "backend/vulkan/VulkanContextFactory.h"

TEST_F(WindowFixture, ShouldCreateAndDestroyAVertexBuffer)
{

    Fox::DContextConfig config;
    config.warningFunction = &WarningAssert;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);
    ASSERT_NE(context, nullptr);

    Fox::DBuffer buf = context->CreateVertexBuffer(10 * 1024 * 1024); // 10mb

    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(buf->Type, Fox::EBufferType::VERTEX_INDEX_BUFFER);

    context->DestroyVertexBuffer(buf);

    delete context;
}
