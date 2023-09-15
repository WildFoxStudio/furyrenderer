#include "RingBufferManager.h"

#include <gtest/gtest.h>

using namespace Fox;

TEST(UnitRingBufferManager, ShouldCorrectlyInitialize)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());

    EXPECT_EQ(ringBuffer.AvailableSpace(), MAX_SIZE);
}

TEST(UnitRingBufferManager2, ShouldCorrectlyCopyValidLengthValue)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    std::string expected{ "HELLO" };

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());
    ringBuffer.Copy(expected.data(), expected.length());

    std::string result(pool.begin(), pool.begin() + expected.length());

    EXPECT_EQ(result, expected);
    const auto expectedAvailableSpace = MAX_SIZE - expected.length();
    EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
}

TEST(UnitRingBufferManager3, ShouldCorrectlyReturnValidPtrToCopyLocation)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    std::string expected{ "HELLO" };

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());

    const auto locationPtr = ringBuffer.Copy(expected.data(), expected.length());

    std::vector<unsigned char> memory;
    memory.resize(expected.length());

    memcpy(memory.data(), pool.data() + locationPtr, expected.length());

    std::string result(memory.begin(), memory.end());

    EXPECT_EQ(result, expected);
    const auto expectedAvailableSpace = MAX_SIZE - expected.length();
    EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
}

TEST(UnitRingBufferManager4, ShouldHaveZeroSpaceLeft)
{
    constexpr uint32_t         MAX_SIZE = 8;
    std::vector<unsigned char> pool;
    pool.resize(MAX_SIZE);

    std::string expected{ "HELLO" };

    RingBufferManager ringBuffer(MAX_SIZE, pool.data());
    ringBuffer.Copy(expected.data(), expected.length());

    std::string foo{ "FOO" };
    ringBuffer.Copy(foo.data(), foo.length());

    EXPECT_EQ(ringBuffer.AvailableSpace(), 0);
    EXPECT_EQ(ringBuffer.DoesFit(1), false);

    ringBuffer.SetTail(1);
    EXPECT_EQ(ringBuffer.AvailableSpace(), 1);
    EXPECT_EQ(ringBuffer.DoesFit(1), true);
}

TEST(UnitRingBufferManager5, BigCopyDoesNotFitShouldWrapAroundToTheBeginning)
{
    constexpr uint32_t MAX_SIZE = 8;
    std::vector<char>  pool;
    pool.resize(MAX_SIZE);

    std::string hello{ "HELLO" };

    RingBufferManager ringBuffer(MAX_SIZE, (unsigned char*)pool.data());

    const auto locationPtr = ringBuffer.Copy(hello.data(), hello.length());

    {
        const auto expectedAvailableSpace = MAX_SIZE - hello.length();
        EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
    }

    ringBuffer.SetTail(4);

    {
        const auto expectedAvailableSpace = 7;
        EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
    }

    EXPECT_EQ(ringBuffer.DoesFit(4), true);
    EXPECT_EQ(ringBuffer.DoesFit(5), false);

    std::string one{ "A" };
    ringBuffer.Copy(one.data(), one.length());
    ringBuffer.SetTail(6);//head same as tail means all free

    EXPECT_EQ(ringBuffer.DoesFit(5), true);

    std::string lorem{ "lorem" };
    ringBuffer.Copy(lorem.data(), lorem.length());

    {
        const auto expectedAvailableSpace = 3;
        EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
    }

    {
        const char*       resultString = "loremA\0\0";
        std::vector<char> result(resultString, resultString + pool.size());

        EXPECT_EQ(pool, result);
    }
}

TEST(UnitRingBufferManager6, ShouldCorrectlyWrapAroundWhenAllocatingTooMuch)
{
    constexpr uint32_t MAX_SIZE = 10;
    std::vector<char>  pool;
    pool.resize(MAX_SIZE);

    RingBufferManager ringBuffer(MAX_SIZE, (unsigned char*)pool.data());

    std::string hello{ "HELLO" };
    std::string world{ "WORLD" };
    std::string nice{ "nice" };

    ringBuffer.Copy(hello.data(), hello.length());

    {
        const auto expectedAvailableSpace = MAX_SIZE - hello.length();
        EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
    }

    const auto lastOffset = ringBuffer.Copy(world.data(), world.length());

    {
        const auto expectedAvailableSpace = MAX_SIZE - hello.length() - world.length();
        EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
    }

    ringBuffer.SetTail(lastOffset);

    {
        const auto expectedAvailableSpace = 5;
        EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
    }

    {
        const char*       resultString = "HELLOWORLD";
        std::vector<char> result(resultString, resultString + strlen(resultString));

        EXPECT_EQ(pool, result);
    }
    ringBuffer.Copy(nice.data(), nice.length());
    {
        const char*       resultString = "niceOWORLD";
        std::vector<char> result(resultString, resultString + strlen(resultString));

        EXPECT_EQ(pool, result);
    }

    {
        const auto expectedAvailableSpace = 1;
        EXPECT_EQ(ringBuffer.AvailableSpace(), expectedAvailableSpace);
    }
}