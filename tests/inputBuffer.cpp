#include <gtest/gtest.h>
#include "input/inputBuffer.hpp"
using namespace input;



TEST(InputBufferTest, InitializedToZero)
{
    InputBuffer inputBuffer;

    // check
    for (std::size_t i = 0; i < INPUT_BUFFER_SIZE; i++)
        EXPECT_EQ(inputBuffer[i], 0);
}


TEST(InputBufferTest, Insert)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000 + 5);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);

    // check
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 4]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 5]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 6]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[std::numeric_limits<tick_t>::max()]), 0xA);
}


TEST(InputBufferTest, InsertBeforeLatest)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000 + 5);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);
    set::timestamp(pin, 1'000'000 + 1);
    set::actions(pin, 0xB);
    inputBuffer.insert(pin);
   
    // check
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 0]), 0);    
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 1]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 2]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 3]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 4]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 5]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 6]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 7]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 8]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 9]), 0xA);
}


TEST(InputBufferTest, InsertAfterLatest)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000 + 5);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);
    set::timestamp(pin, 1'000'000 + 8);
    set::actions(pin, 0xB);
    inputBuffer.insert(pin);

    // check
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 0]), 0);    
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 1]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 2]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 3]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 4]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 5]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 6]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 7]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 8]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 9]), 0xB);
}


TEST(InputBufferTest, InsertAtLatest)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000 + 5);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);
    set::timestamp(pin, 1'000'000 + 5);
    set::actions(pin, 0xB);
    inputBuffer.insert(pin);

    // check
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 0]), 0);    
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 1]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 2]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 3]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 4]), 0);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 5]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 6]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 7]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 8]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 9]), 0xB);
}



TEST(InputBufferTest, InsertAtLatestPlusSize)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000 + 5);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);
    set::timestamp(pin, 1'000'000 + 5 + INPUT_BUFFER_SIZE);
    set::actions(pin, 0xB);
    inputBuffer.insert(pin);
    
    // check
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 0 + INPUT_BUFFER_SIZE]), 0xA);    
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 1 + INPUT_BUFFER_SIZE]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 2 + INPUT_BUFFER_SIZE]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 3 + INPUT_BUFFER_SIZE]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 4 + INPUT_BUFFER_SIZE]), 0xA);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 5 + INPUT_BUFFER_SIZE]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 6 + INPUT_BUFFER_SIZE]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 7 + INPUT_BUFFER_SIZE]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 8 + INPUT_BUFFER_SIZE]), 0xB);
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 9 + INPUT_BUFFER_SIZE]), 0xB);
}


TEST(InputBufferTest, InsertTooOld)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);
    set::timestamp(pin, 0);
    set::actions(pin, 0xB);

    // check
#if DEBUG == true
    EXPECT_THROW(inputBuffer.insert(pin), std::invalid_argument);
#else
    EXPECT_NO_THROW(inputBuffer.insert(pin));
#endif
}


TEST(InputBufferTest, ReadBeforeLatest)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000 + 5);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);

    // check
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 2]), 0);
}


TEST(InputBufferTest, ReadAfterLatest)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000 + 5);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);
    
    // check
    EXPECT_EQ(get::actions(inputBuffer[1'000'000 + 7]), 0xA);
}


TEST(InputBufferTest, ReadTooOld)
{
    InputBuffer inputBuffer;
    PlayerInput pin;

    // prepare
    set::timestamp(pin, 1'000'000);
    set::actions(pin, 0xA);
    inputBuffer.insert(pin);

    // check
#if DEBUG == true
    EXPECT_THROW(inputBuffer[0], std::invalid_argument);
#else
    EXPECT_NO_THROW(inputBuffer[0]);
#endif
}
