#pragma once
#include <thread>
#include "net.hpp"
#include "SDL3_net/SDL_net.h"



namespace net
{

    class Connection
    {
    public:
        void spin(std::chrono::system_clock::duration period);
        void enqueue(Message message);

    private:
        std::array<Message, 1024> _sendQueue;
        std::size_t _queueBegin;
        std::size_t _queueEnd;

        void _send();
    };


    class ConnectionManager
    {
    public:
        ConnectionManager()
        {
            NET_Init();
        }

        ~ConnectionManager()
        {
            NET_Quit();
        }
        
        void startDiscovery();
        void endDiscovery(bool abort);

    private:
    };

};  // end namespace net
