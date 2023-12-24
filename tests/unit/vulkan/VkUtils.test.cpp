#include "backend/vulkan/UtilsVK.h"

#include <gtest/gtest.h>

#include <algorithm>

using namespace VkUtils;

TEST(UnitFilterInclusive, ShouldFilterAsExpected)
{

    const std::vector<const char*> source{ "hello", "world", "foo" };
    const std::vector<const char*> included{ "hello", "Cat" };
    const auto                     onlyIncluded = filterInclusive(source, included);

    ASSERT_EQ(onlyIncluded.size(), 1);

    const auto found = std::find_if(onlyIncluded.cbegin(), onlyIncluded.cend(), [&included](const char* a) {
        // cmp strings
        return strcmp(a, included[0]) == 0;
    });
    ASSERT_NE(found, onlyIncluded.end());
}

TEST(UnitFilterExclusive, ShouldFilterAsExpected)
{

    const std::vector<const char*> source{ "hello", "world", "foo" };
    const std::vector<const char*> included{ "hello", "Cat" };
    const auto                     onlyIncluded = filterExclusive(source, included);

    {
        const auto found = std::find_if(onlyIncluded.begin(), onlyIncluded.end(), [&source](const char* a) { return strcmp(a, source[1]) == 0; });
        ASSERT_NE(found, onlyIncluded.end());
    }
    {
        const auto found = std::find_if(onlyIncluded.begin(), onlyIncluded.end(), [&source](const char* a) { return strcmp(a, source[2]) == 0; });
        ASSERT_NE(found, onlyIncluded.end());
    }
}

TEST(UnitConvertQueueTypeToVkFlags, ShouldConvertCorrectlyFlags)
{
    using namespace Fox;

    {
        const VkFlags flag = convertQueueTypeToVkFlags(EQueueType::QUEUE_GRAPHICS);
        ASSERT_TRUE(flag & VK_QUEUE_GRAPHICS_BIT && (~flag & VK_QUEUE_GRAPHICS_BIT) == 0);
    }
    {
        const VkFlags flag = convertQueueTypeToVkFlags(EQueueType::QUEUE_COMPUTE);
        ASSERT_TRUE(flag & VK_QUEUE_COMPUTE_BIT && (~flag & VK_QUEUE_COMPUTE_BIT) == 0);
    }
    {
        const VkFlags flag = convertQueueTypeToVkFlags(EQueueType::QUEUE_TRANSFER);
        ASSERT_TRUE(flag & VK_QUEUE_TRANSFER_BIT && (~flag & VK_QUEUE_TRANSFER_BIT) == 0);
    }
}

TEST(UnitFindQueueFromQueueFlags, ShouldReturnCorrectQueueFamilyAndIndex)
{
    using namespace Fox;

    {
        // NVIDIA GeForce GTX 750 Ti  GPU queue families
        VkQueueFamilyProperties families[3]{};
        families[0].queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        families[0].queueCount = 16;

        families[1].queueFlags = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;
        families[1].queueCount = 1;

        families[2].queueFlags = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_VIDEO_DECODE_BIT_KHR;
        families[2].queueCount = 1;

        uint32_t queueIndexCreatedCount[3]{};
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_GRAPHICS_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_COMPUTE_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_TRANSFER_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 1);
            ASSERT_EQ(queueIndex, 0);
        }
        // Since previous call the queue index was taken now it should return second family that has transfer flag
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_TRANSFER_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 2);
            ASSERT_EQ(queueIndex, 0);
        }
        // Since previous call the queue index was taken now it should return general purpose queue
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_TRANSFER_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
    }

    {
        // NVIDIA GeForce GTX 1080 Ti  GPU queue families
        VkQueueFamilyProperties families[3]{};
        families[0].queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        families[0].queueCount = 16;

        families[1].queueFlags = VK_QUEUE_TRANSFER_BIT;
        families[1].queueCount = 2;

        families[2].queueFlags = VK_QUEUE_COMPUTE_BIT;
        families[2].queueCount = 8;

        uint32_t queueIndexCreatedCount[3]{};
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_GRAPHICS_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
        /*Should enumerate all queue indices*/
        for (uint32_t i = 0; i < families[2].queueCount; i++)
            {
                uint32_t   queueFamily{};
                uint32_t   queueIndex{};
                const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_COMPUTE_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
                ASSERT_TRUE(found);
                ASSERT_EQ(queueFamily, 2);
                ASSERT_EQ(queueIndex, i);
            }
        /*Should enumerate all queue indices*/
        for (uint32_t i = 0; i < families[1].queueCount; i++)
            {
                uint32_t   queueFamily{};
                uint32_t   queueIndex{};
                const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_TRANSFER_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
                ASSERT_TRUE(found);
                ASSERT_EQ(queueFamily, 1);
                ASSERT_EQ(queueIndex, i);
            }
        // Should return general purpose queue because we already queries all queues of this type
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_TRANSFER_BIT, families, sizeof(families) / sizeof(families[0]), queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
    }

    {
        // Intel(R) UHD Graphic  GPU queue families
        VkQueueFamilyProperties families[1]{};
        families[0].queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;
        families[0].queueCount = 1;

        uint32_t queueIndexCreatedCount[1]{};
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_GRAPHICS_BIT, families, 1, queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_COMPUTE_BIT, families, 1, queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_TRANSFER_BIT, families, 1, queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
        {
            uint32_t   queueFamily{};
            uint32_t   queueIndex{};
            const bool found = findQueueWithFlags(&queueFamily, &queueIndex, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, families, 1, queueIndexCreatedCount);
            ASSERT_TRUE(found);
            ASSERT_EQ(queueFamily, 0);
            ASSERT_EQ(queueIndex, 0);
        }
    }
}
