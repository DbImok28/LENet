#pragma once
#include "NetBufferBasedEventManager.hpp"

namespace LimeEngine::Net
{
    class NetPollBuffer;

    template<typename TNetDataHandler, typename TNetEventHandler = NetEventHandler>
    using NetPollEventManager = NetBufferBasedEventManager<NetPollBuffer, TNetDataHandler, TNetEventHandler>;

	struct PollFD
	{
        explicit PollFD(NativeSocket fd, SHORT events = POLLRDNORM, SHORT revents = 0) : fd(fd), events(events), revents(revents) {}

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
        bool IsChanged() const
        {
            return revents != 0;
        }

        void SetFlag(SHORT flags = POLLRDNORM)
        {
            events = flags;
        }

    private:
		NativeSocket fd;
		SHORT events;
		SHORT revents;
	};

    class NetPollBuffer
    {
    public:
        bool Add(NativeSocket fd)
        {
            pollFDs.emplace_back(fd, POLLRDNORM, 0);
            return true;
        }
        void Remove(size_t index)
        {
            pollFDs.erase(std::begin(pollFDs) + index);
        }

        int WaitForEvents(uint32_t timeout)
        {
            int result = WSAPoll(reinterpret_cast<WSAPOLLFD*>(pollFDs.data()), pollFDs.size(), timeout);
            if (result < 0)
            {
                LENET_LAST_ERROR_MSG("Can't to poll");
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
            std::ostringstream oss;
            oss << "[Poll] Expected:";
            for (auto& pollFD : pollFDs)
            {
                auto& wsaPollFD = *reinterpret_cast<const WSAPOLLFD*>(&pollFD);
                oss << '[';
                oss << ((wsaPollFD.events & POLLRDNORM) ? "R" : "");
                oss << ((wsaPollFD.events & POLLWRNORM) ? "W" : "");
                oss << ((wsaPollFD.events & POLLERR) ? "E" : "");
                oss << ((wsaPollFD.events & POLLHUP) ? "D" : "");
                oss << ']';
            }
            oss << " Actual:";
            for (auto& pollFD : pollFDs)
            {
                oss << '[';
                oss << ((pollFD.CheckRead()) ? "R" : "");
                oss << ((pollFD.CheckWrite()) ? "W" : "");
                oss << ((pollFD.CheckExcept()) ? "E" : "");
                oss << ((pollFD.CheckDisconnect()) ? "D" : "");
                oss << ']';
            }
            NetLogger::LogCore(oss.str());
        }

    private:
        std::vector<PollFD> pollFDs;
    };
}
