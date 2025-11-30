#pragma once
#include <inttypes.h>
#include <array>
#include <chrono>



namespace net
{

    using Message = uint64_t;


    enum MessageType : Message
    {
        DISCOVER = 0,
        INPUT = 3,
        MAX_ENUM = 15
    };


    enum BitMask : Message
    {
        HOST_ID         = 0xFF00'0000'0000'0000,
        MESSAGE_TYPE    = 0x00F0'0000'0000'0000,
        PAYLOAD         = 0x000F'FFFF'FFFF'FFFF
    };


    namespace set
    {

        inline void hostID(Message& message, uint8_t id)
            { message = (message & ~BitMask::HOST_ID) | ((Message(id) << 56) & BitMask::HOST_ID); }
        
        inline void messageType(Message& message, MessageType type)
            { message = (message & ~BitMask::MESSAGE_TYPE) | ((Message(type) << 52) & BitMask::MESSAGE_TYPE); }
        
        inline void payload(Message& message, Message payload)
            { message = (message & ~BitMask::PAYLOAD) | (payload & BitMask::PAYLOAD); }
    
    };  // end namespace set


    namespace get
    {

        inline uint8_t hostID(const Message message)
            { return (message & BitMask::HOST_ID) >> 56; }
        
        inline MessageType messageType(const Message message)
            { return MessageType((message & BitMask::MESSAGE_TYPE) >> 52); }
        
        inline Message payload(const Message message)
            { return message & BitMask::PAYLOAD; }
    
    };  // end namespace get

};  // end namespace net
