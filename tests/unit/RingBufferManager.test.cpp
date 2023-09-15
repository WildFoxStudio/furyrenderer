#include "RingBufferManager.h"

#include <gtest/gtest.h>

using namespace Fox;

TEST(UnitRingBufferManager, ShouldCorrectlyInitialize)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());

    EXPECT_EQ(ringBuffer.Capacity(), MAX_SIZE);
}

TEST(UnitRingBufferManager2, ShouldCorrectlyCopyValidLengthValue)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    std::string expected{ "HELLO" };

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());
    ringBuffer.Push(expected.data(), expected.length());

    std::string result(pool.begin(), pool.begin() + expected.length());

    EXPECT_EQ(result, expected);
    const auto expectedSize = expected.length();
    EXPECT_EQ(ringBuffer.Size(), expectedSize);
}

TEST(UnitRingBufferManager3, ShouldCorrectlyReturnValidPtrToCopyLocation)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    std::string expected{ "HELLO" };

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());

    const auto locationPtr = ringBuffer.Push(expected.data(), expected.length());

    std::vector<unsigned char> memory;
    memory.resize(expected.length());

    memcpy(memory.data(), pool.data() + locationPtr, expected.length());

    std::string result(memory.begin(), memory.end());

    EXPECT_EQ(result, expected);
    const auto expectedSize = expected.length();
    EXPECT_EQ(ringBuffer.Size(), expectedSize);
}

TEST(UnitRingBufferManager4, ShouldHaveZeroSpaceLeft)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    std::string expected{ "HELLO" };

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());
    ringBuffer.Push(expected.data(), expected.length());

    std::string foo{ "FOO" };
    ringBuffer.Push(foo.data(), foo.length());

    EXPECT_EQ(ringBuffer.Size(), MAX_SIZE);
    EXPECT_EQ(ringBuffer.Capacity(), 0);

    ringBuffer.Pop(expected.length());
    EXPECT_EQ(ringBuffer.Size(), foo.length());
    EXPECT_EQ(ringBuffer.Capacity(), MAX_SIZE - foo.length());
}

TEST(UnitRingBufferManager4, ShouldFillEmptyCapacity)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    std::string expected{ "HELLO" };

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());
    ringBuffer.Push(expected.data(), expected.length());

    std::string foo{ "A" };
    ringBuffer.Push(foo.data(), foo.length());
    ringBuffer.Pop(expected.length());

    EXPECT_EQ(ringBuffer.Capacity(), 2);
    EXPECT_EQ(ringBuffer.Size(), 1);

    ringBuffer.Push(nullptr, 2);

    EXPECT_EQ(ringBuffer.Capacity(), 5);
    EXPECT_EQ(ringBuffer.Size(), 3);

    ringBuffer.Push(expected.data(), expected.length());

    EXPECT_EQ(ringBuffer.Capacity(), 0);
    EXPECT_EQ(ringBuffer.Size(), MAX_SIZE);

    ringBuffer.Pop(foo.length());
    ringBuffer.Pop(2);
    ringBuffer.Pop(expected.length());

    EXPECT_EQ(ringBuffer.Capacity(), MAX_SIZE);
    EXPECT_EQ(ringBuffer.Size(), 0);
}

// TEST(UnitRingBufferManager5, BigCopyDoesNotFitShouldWrapAroundToTheBeginning)
//{
//     constexpr uint32_t MAX_SIZE = 8;
//     std::vector<char>  pool;
//     pool.resize(MAX_SIZE);
//
//     std::string hello{ "HELLO" };
//
//     RingBufferManager ringBuffer(MAX_SIZE, (unsigned char*)pool.data());
//
//     const auto locationPtr = ringBuffer.Push(hello.data(), hello.length());
//
//     {
//         const auto expectedAvailableSpace = MAX_SIZE - hello.length();
//         EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
//     }
//     //allocated 4
//     ringBuffer.Pop(4);
//
//     {
//         const auto expectedAvailableSpace = 7;
//         EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
//     }
//
//     EXPECT_EQ(ringBuffer.Capacity(4), true);
//     EXPECT_EQ(ringBuffer.Capacity(5), false);
//
//     std::string one{ "A" };
//     ringBuffer.Push(one.data(), one.length());
//     ringBuffer.Pop(1); // head same as tail means all free
//
//     EXPECT_EQ(ringBuffer.Capacity(5), true);
//
//     std::string lorem{ "lorem" };
//     ringBuffer.Push(lorem.data(), lorem.length());
//
//     {
//         const auto expectedAvailableSpace = 3;
//         EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
//     }
//
//     {
//         const char*       resultString = "loremA\0\0";
//         std::vector<char> result(resultString, resultString + pool.size());
//
//         EXPECT_EQ(pool, result);
//     }
// }
//
// TEST(UnitRingBufferManager6, ShouldCorrectlyWrapAroundWhenAllocatingTooMuch)
//{
//     constexpr uint32_t MAX_SIZE = 10;
//     std::vector<char>  pool;
//     pool.resize(MAX_SIZE);
//
//     RingBufferManager ringBuffer(MAX_SIZE, (unsigned char*)pool.data());
//
//     std::string hello{ "HELLO" };
//     std::string world{ "WORLD" };
//     std::string nice{ "nice" };
//
//     ringBuffer.Push(hello.data(), hello.length());
//
//     {
//         const auto expectedAvailableSpace = MAX_SIZE - hello.length();
//         EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
//     }
//
//     const auto lastOffset = ringBuffer.Push(world.data(), world.length());
//
//     {
//         const auto expectedAvailableSpace = MAX_SIZE - hello.length() - world.length();
//         EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
//     }
//
//     ringBuffer.Pop(hello.length() + world.length());
//
//     {
//         const auto expectedAvailableSpace = 5;
//         EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
//     }
//
//     {
//         const char*       resultString = "HELLOWORLD";
//         std::vector<char> result(resultString, resultString + strlen(resultString));
//
//         EXPECT_EQ(pool, result);
//     }
//     ringBuffer.Push(nice.data(), nice.length());
//     {
//         const char*       resultString = "niceOWORLD";
//         std::vector<char> result(resultString, resultString + strlen(resultString));
//
//         EXPECT_EQ(pool, result);
//     }
//
//     {
//         const auto expectedAvailableSpace = 1;
//         EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
//     }
// }