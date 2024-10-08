#pragma once
#include "NetConnection.hpp"
#include "NetSockets.hpp"

namespace LimeEngine::Net
{
    struct SelectFD
    {
        SelectFD(NetSocket &fd, bool read = false, bool written = false, bool excepted = false) : fd(fd), read(read), written(written), excepted(excepted) {}

        bool CheckRead() const
        {
            return read;
        }
        bool CheckWrite() const
        {
            return written;
        }
        bool CheckExcept() const
        {
            return excepted;
        }
        bool CheckDisconnect() const
        {
            return false;
        }

    public:
        NetSocket& fd;

    private:
        bool read;
        bool written;
        bool excepted;
    };

    class NetSelectManager
    {
    public:
        NetSelectManager() noexcept
        {
            FD_ZERO(&readFDs);
            FD_ZERO(&writeFDs);
            FD_ZERO(&exceptFDs);
        }
        bool AddSocket(NetSocket&& socket)
        {
            SOCKET fd = socket.GetSocket();
            if (fd >= FD_SETSIZE)
            {
                LENET_MSG_ERROR(std::format("Unable to add socket for select. Maximum allowed socket value = {}(FD_SETSIZE), current value {}", FD_SETSIZE, fd));
                return false;
            }

            FD_SET(fd, &readFDs);
            //FD_SET(fd, &writeFDs);
            FD_SET(fd, &exceptFDs);

#ifndef LENET_WIN32
            if (fd > largestSocket)
            {
                largestSocket = fd;
            }
#endif
            sockets.emplace_back(std::move(socket));
            return true;
        }
        void RemoveSocket(size_t index)
        {
            SOCKET fd = sockets[index].GetSocket();
            FD_CLR(fd, &readFDs);
            FD_CLR(fd, &writeFDs);
            FD_CLR(fd, &exceptFDs);

#ifndef LENET_WIN32
            if (largestSocket == fd)
            {
                auto& fds = *reinterpret_cast<std::vector<SOCKET>*>(&sockets);
                largestSocket = *std::max_element(std::begin(fds), std::end(fds));
                //largestSocket = std::max_element(std::begin(sockets), std::end(sockets), [](auto& socket){ return socket.GetSocket();})->GetSocket();
            }
#endif
            sockets.erase(std::begin(sockets) + index);
        }
        int WaitForEvents(uint32_t timeout)
        {
            readFDsCopy = readFDs;
            writeFDsCopy = writeFDs;
            exceptFDsCopy = exceptFDs;

            // TODO: add timeout
            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = timeout;

            int result = select(largestSocket, &readFDsCopy, &writeFDsCopy, &exceptFDsCopy, &tv);
            if (result < 0)
            {
                // EWOULDBLOCK
                LENET_ERROR(WSAGetLastError(), "Can't to select");
                return false;
            }
            //int result = select(largestSocket, &readFDsCopy, &writeFDsCopy, &exceptFDsCopy, nullptr);
            return result;
        }

        SelectFD At(size_t index)
        {
            auto& socket = sockets[index];
            auto fd = socket.GetSocket();
            return {socket, static_cast<bool>(FD_ISSET(fd, &readFDsCopy)), static_cast<bool>(FD_ISSET(fd, &writeFDsCopy)),static_cast<bool>(FD_ISSET(fd, &exceptFDsCopy))};
        }

        size_t Count() const
        {
            return sockets.size();
        }
        bool Empty() const
        {
            return sockets.empty();
        }
        void SetWriteFlag(size_t index)
        {
            FD_SET(sockets[index].GetSocket(), &writeFDs);
        }
        void ResetWriteFlag(size_t index)
        {
            FD_CLR(sockets[index].GetSocket(), &writeFDs);
        }

        void Log() const
        {
            std::cout << "[Select] Expected:";
            for (auto& socket : sockets)
            {
                std::cout << '[';
                std::cout << ((FD_ISSET(socket.GetSocket(), &readFDs)) ? "R" : "");
                std::cout << ((FD_ISSET(socket.GetSocket(), &writeFDs)) ? "W" : "");
                std::cout << ((FD_ISSET(socket.GetSocket(), &exceptFDs)) ? "E" : "");
                std::cout << ']';
            }
            std::cout << " Actual:";
            for (auto& socket : sockets)
            {
                std::cout << '[';
                std::cout << ((FD_ISSET(socket.GetSocket(), &readFDsCopy)) ? "R" : "");
                std::cout << ((FD_ISSET(socket.GetSocket(), &writeFDsCopy)) ? "W" : "");
                std::cout << ((FD_ISSET(socket.GetSocket(), &exceptFDsCopy)) ? "E" : "");
                std::cout << ']';
            }
            std::cout << std::endl;
        }

    private:
        std::vector<NetSocket> sockets;
        //std::array<NetSocket, FD_SETSIZE> sockets;
        fd_set readFDs;
        fd_set writeFDs;
        fd_set exceptFDs;

        fd_set readFDsCopy;
        fd_set writeFDsCopy;
        fd_set exceptFDsCopy;

#ifdef LENET_WIN32
        // Windows ignores nfds
        static constexpr int largestSocket = FD_SETSIZE;
#else
        int largestSocket = 0;
#endif
    };
}
