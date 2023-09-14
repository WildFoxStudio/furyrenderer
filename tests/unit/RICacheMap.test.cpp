#include "RICacheMap.h"

#include <gtest/gtest.h>

using namespace Fox;

struct Uint32EqualFn
{
    bool operator()(const uint32_t& lhr, const uint32_t& rhs) const { return lhr == rhs; };
};

TEST(UnitRICacheMap, ShouldAddAndFindAnElement)
{
    RICacheMap<uint32_t, uint32_t*, std::hash<uint32_t>, Uint32EqualFn> cacheMap;

    cacheMap.Add(123456789, (uint32_t*)0xff);
    cacheMap.Add(23456789, (uint32_t*)0xaa);
    cacheMap.Add(3456789, (uint32_t*)0xbb);

    EXPECT_EQ(cacheMap.Find(123456789), (uint32_t*)0xff);
    EXPECT_EQ(cacheMap.Find(23456789), (uint32_t*)0xaa);
    EXPECT_EQ(cacheMap.Find(3456789), (uint32_t*)0xbb);
}

TEST(UnitRICacheMap2, SizeShouldBeAsExpected)
{
    RICacheMap<uint32_t, uint32_t*, std::hash<uint32_t>, Uint32EqualFn> cacheMap;

    cacheMap.Add(123456789, (uint32_t*)0xff);
    cacheMap.Add(23456789, (uint32_t*)0xaa);
    cacheMap.Add(3456789, (uint32_t*)0xbb);

    EXPECT_EQ(cacheMap.Size(), 3);
}

TEST(UnitRICacheMap3, ShouldRemoveCorrectElements)
{
    RICacheMap<uint32_t, uint32_t*, std::hash<uint32_t>, Uint32EqualFn> cacheMap;

    cacheMap.Add(123456789, (uint32_t*)0xff);
    cacheMap.Add(23456789, (uint32_t*)0xaa);
    cacheMap.Add(3456789, (uint32_t*)0xbb);

    cacheMap.EraseByValue((uint32_t*)0xff);
    EXPECT_EQ(cacheMap.Size(), 2);
    EXPECT_EQ(cacheMap.Find(23456789), (uint32_t*)0xaa);
    EXPECT_EQ(cacheMap.Find(3456789), (uint32_t*)0xbb);

    cacheMap.EraseByValue((uint32_t*)0xbb);
    EXPECT_EQ(cacheMap.Size(), 1);
    EXPECT_EQ(cacheMap.Find(23456789), (uint32_t*)0xaa);

    cacheMap.EraseByValue((uint32_t*)0xaa);
    EXPECT_EQ(cacheMap.Size(), 0);
}

TEST(UnitRICacheMap4, ShouldClearAllElements)
{
    RICacheMap<uint32_t, uint32_t*, std::hash<uint32_t>, Uint32EqualFn> cacheMap;

    cacheMap.Add(123456789, (uint32_t*)0xff);
    cacheMap.Add(23456789, (uint32_t*)0xaa);
    cacheMap.Add(3456789, (uint32_t*)0xbb);

    EXPECT_EQ(cacheMap.Size(), 3);
    cacheMap.Clear();
    EXPECT_EQ(cacheMap.Size(), 0);
}
