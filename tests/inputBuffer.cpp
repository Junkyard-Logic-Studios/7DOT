#include <gtest/gtest.h>
#include "constants.hpp"
#include "input/inputBuffer.hpp"
#include <iostream>



TEST(InputBufferTest, InitializedToZero)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;

    // check
    for (std::size_t i = 0; i < inputBuffer.size(); i++)
        EXPECT_EQ(inputBuffer[i], 0);
}


TEST(InputBufferTest, Insert)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000 + 5, 0xA);

    // check
    EXPECT_EQ(inputBuffer[1'000'000 + 4], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 5], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 6], 0xA);
    EXPECT_EQ(inputBuffer[std::numeric_limits<int64_t>::max()], 0xA);
}


TEST(InputBufferTest, InsertBeforeLatest)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000 + 5, 0xA);
    inputBuffer.insert(1'000'000 + 1, 0xB);
    
    // check
    EXPECT_EQ(inputBuffer[1'000'000 + 0], 0);    
    EXPECT_EQ(inputBuffer[1'000'000 + 1], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 2], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 3], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 4], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 5], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 6], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 7], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 8], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 9], 0xA);
}


TEST(InputBufferTest, InsertAfterLatest)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000 + 5, 0xA);
    inputBuffer.insert(1'000'000 + 8, 0xB);
    
    // check
    EXPECT_EQ(inputBuffer[1'000'000 + 0], 0);    
    EXPECT_EQ(inputBuffer[1'000'000 + 1], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 2], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 3], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 4], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 5], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 6], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 7], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 8], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 9], 0xB);
}


TEST(InputBufferTest, InsertAtLatest)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000 + 5, 0xA);
    inputBuffer.insert(1'000'000 + 5, 0xB);
    
    // check
    EXPECT_EQ(inputBuffer[1'000'000 + 0], 0);    
    EXPECT_EQ(inputBuffer[1'000'000 + 1], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 2], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 3], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 4], 0);
    EXPECT_EQ(inputBuffer[1'000'000 + 5], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 6], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 7], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 8], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 9], 0xB);
}


TEST(InputBufferTest, InsertAtLatestPlusSize)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000 + 5,  0xA);
    inputBuffer.insert(1'000'000 + 15, 0xB);
    
    // check
    EXPECT_EQ(inputBuffer[1'000'000 + 10], 0xA);    
    EXPECT_EQ(inputBuffer[1'000'000 + 11], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 12], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 13], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 14], 0xA);
    EXPECT_EQ(inputBuffer[1'000'000 + 15], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 16], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 17], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 18], 0xB);
    EXPECT_EQ(inputBuffer[1'000'000 + 19], 0xB);
}


TEST(InputBufferTest, InsertTooOld)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000, 0xA);

    // check
#ifdef DEBUG
    EXPECT_THROW(inputBuffer.insert(0, 0xB), std::invalid_argument);
#else
    EXPECT_NO_THROW(inputBuffer.insert(0, 0xB));
#endif
}


TEST(InputBufferTest, ReadBeforeLatest)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000 + 5, 0xA);

    // check
    EXPECT_EQ(inputBuffer[1'000'000 + 2], 0);
}


TEST(InputBufferTest, ReadAfterLatest)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000 + 5, 0xA);

    // check
    EXPECT_EQ(inputBuffer[1'000'000 + 7], 0xA);
}


TEST(InputBufferTest, ReadTooOld)
{
    // prepare
    input::InputBufferT<int64_t, input::PlayerInput, 10> inputBuffer;
    inputBuffer.insert(1'000'000, 0xA);

    // check
#ifdef DEBUG
    EXPECT_THROW(inputBuffer[0], std::invalid_argument);
#else
    EXPECT_NO_THROW(inputBuffer[0]);
#endif
}
