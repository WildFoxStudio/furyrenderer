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