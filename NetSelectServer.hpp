#pragma once
#include "NetConnection.hpp"
#include "NetSockets.hpp"

namespace LimeEngine::Net
{
    struct SelectFD
    {
        explicit SelectFD(SOCKET fd, bool read = false, bool written = false, bool excepted = false) : fd(fd), read(read), written(written), excepted(excepted) {}

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
        bool IsChanged() const
        {
            return read || written || excepted;
        }

    private:
        SOCKET fd;
        bool read;
        bool written;
        bool excepted;
    };

    class NetSelectBuffer
    {
    public:
        NetSelectBuffer() noexcept
        {
            FD_ZERO(&readFDs);
            FD_ZERO(&writeFDs);
            FD_ZERO(&exceptFDs);
        }
        bool Add(SOCKET fd)
        {
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
            sockets.emplace_back(fd);
            return true;
        }
        void Remove(size_t index)
        {
            SOCKET fd = sockets[index];
            FD_CLR(fd, &readFDs);
            FD_CLR(fd, &writeFDs);
            FD_CLR(fd, &exceptFDs);

#ifndef LENET_WIN32
            if (largestSocket == fd)
            {
                largestSocket = *std::max_element(std::begin(sockets), std::end(sockets));
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
            return result;
        }

        SelectFD At(size_t index)
        {
            SOCKET fd = sockets[index];
            return SelectFD{fd, static_cast<bool>(FD_ISSET(fd, &readFDsCopy)), static_cast<bool>(FD_ISSET(fd, &writeFDsCopy)), static_cast<bool>(FD_ISSET(fd, &exceptFDsCopy))};
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
            FD_SET(sockets[index], &writeFDs);
        }
        void ResetWriteFlag(size_t index)
        {
            FD_CLR(sockets[index], &writeFDs);
        }

        void Log() const
        {
            std::ostringstream oss;
            oss << "[Select] Expected:";
            for (auto& socket : sockets)
            {
                oss << '[';
                oss << ((FD_ISSET(socket, &readFDs)) ? "R" : "");
                oss << ((FD_ISSET(socket, &writeFDs)) ? "W" : "");
                oss << ((FD_ISSET(socket, &exceptFDs)) ? "E" : "");
                oss << ']';
            }
            oss << " Actual:";
            for (auto& socket : sockets)
            {
                oss << '[';
                oss << ((FD_ISSET(socket, &readFDsCopy)) ? "R" : "");
                oss << ((FD_ISSET(socket, &writeFDsCopy)) ? "W" : "");
                oss << ((FD_ISSET(socket, &exceptFDsCopy)) ? "E" : "");
                oss << ']';
            }
            NetLogger::LogCore(oss.str());
        }

    private:
        std::vector<SOCKET> sockets;
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
