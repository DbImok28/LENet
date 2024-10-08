#pragma once
#include "NetConnection.hpp"
#include "NetSockets.hpp"

namespace LimeEngine::Net
{
	struct PollFD
	{
        explicit PollFD(NetSocket&& fd, SHORT events = POLLRDNORM, SHORT revents = 0) : fd(std::move(fd)), events(events), revents(revents) {}

        bool CheckRead() const
        {
            return revents & POLLRDNORM;
        }
        bool CheckWrite() const
        {
            return revents & POLLWRNORM;
        }
        bool CheckExcept() const
        {
            return revents & POLLERR;
        }
        bool CheckDisconnect() const
        {
            return revents & POLLHUP;
        }

        void SetFlag(SHORT flags = POLLRDNORM)
        {
            events = flags;
        }

    public:
		NetSocket fd;

    private:
		SHORT events;
		SHORT revents;
	};

    class NetPollManager
    {
    public:
        bool AddSocket(NetSocket&& socket)
        {
            pollFDs.emplace_back(std::move(socket), POLLRDNORM, 0);
            return true;
        }
        void RemoveSocket(size_t index)
        {
            pollFDs.erase(std::begin(pollFDs) + index);
        }

        int WaitForEvents(uint32_t timeout)
        {
            int result = WSAPoll(reinterpret_cast<WSAPOLLFD*>(pollFDs.data()), pollFDs.size(), timeout);
            if (result < 0)
            {
                LENET_ERROR(WSAGetLastError(), "Can't to poll");
                return 0;
            }
            return result;
        }

        void SetWriteFlag(size_t index)
        {
            pollFDs[index].SetFlag(POLLRDNORM | POLLWRNORM);
        }
        void ResetWriteFlag(size_t index)
        {
            pollFDs[index].SetFlag(POLLRDNORM);
        }

        size_t Count() const
        {
            return pollFDs.size();
        }
        bool Empty() const
        {
            return pollFDs.empty();
        }
        PollFD& At(size_t index)
        {
            return pollFDs[index];
        }

        void Log() const
        {
            std::cout << "[Poll] Expected:";
            for (auto& pollFD : pollFDs)
            {
                auto& wsaPollFD = *reinterpret_cast<const WSAPOLLFD*>(&pollFD);
                std::cout << '[';
                std::cout << ((wsaPollFD.events & POLLRDNORM) ? "R" : "");
                std::cout << ((wsaPollFD.events & POLLWRNORM) ? "W" : "");
                std::cout << ((wsaPollFD.events & POLLERR) ? "E" : "");
                std::cout << ((wsaPollFD.events & POLLHUP) ? "D" : "");
                std::cout << ']';
            }
            std::cout << " Actual:";
            for (auto& pollFD : pollFDs)
            {
                std::cout << '[';
                std::cout << ((pollFD.CheckRead()) ? "R" : "");
                std::cout << ((pollFD.CheckWrite()) ? "W" : "");
                std::cout << ((pollFD.CheckExcept()) ? "E" : "");
                std::cout << ((pollFD.CheckDisconnect()) ? "D" : "");
                std::cout << ']';
            }
            std::cout << std::endl;
        }

    private:
        std::vector<PollFD> pollFDs;
    };
}
