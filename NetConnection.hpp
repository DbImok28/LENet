#pragma once
#include <string>
#include <queue>
#include <functional>

namespace LimeEngine::Net
{
	class NetSendMessage
	{
	public:
        NetSendMessage(std::string&& msg) : msg(std::move(msg)) {}
        NetSendMessage(const std::string& msg) : msg(msg) {}

    public:
        std::string msg;
        bool sended = false;
	};

    class NetReceivedMessage
    {
    public:
        NetReceivedMessage(std::string&& msg) : msg(std::move(msg)) {}
        NetReceivedMessage(const std::string& msg) : msg(msg) {}

    public:
        std::string msg;
    };

    enum class NetStatus
    {
        Init,
        Error,
        Opened,
        MarkForClose,
        Closed
    };

	class NetConnection
	{
	public:
		NetConnection()
        {
            static uint16_t idCounter = 0;
            Id = idCounter++;
        }
        ~NetConnection()
        {
            if (status == NetStatus::MarkForClose)
            {
                onDisconnect(*this);
            }
        }

        bool operator==(const NetConnection &rhs) const
        {
            return Id == rhs.Id;
        }

        bool operator!=(const NetConnection &rhs) const
        {
            return !(rhs == *this);
        }

        void Send(const std::string& message)
		{
			messagesToSend.emplace(message);
		}
		bool Update()
		{
			while (!receivedMessages.empty())
			{
				onMessage(*this, receivedMessages.back());
				receivedMessages.pop();
			}
			if (status == NetStatus::MarkForClose)
			{
				onDisconnect(*this);
                status = NetStatus::Closed;
				return false;
			}
			return true;
		}

		void OnMessage(const std::function<void(const NetConnection&, const NetReceivedMessage&)>& handler)
		{
			onMessage = handler;
		}
		void OnDisconnect(const std::function<void(const NetConnection&)>& handler)
		{
			onDisconnect = handler;
		}

		void ChangeStateToClose()
		{
			status = NetStatus::MarkForClose;
		}

        uint16_t GetId() const noexcept
        {
            return Id;
        }

		std::queue<NetSendMessage> messagesToSend;
		std::queue<NetReceivedMessage> receivedMessages;

	private:
		std::function<void(const NetConnection&, const NetReceivedMessage&)> onMessage;
		std::function<void(const NetConnection&)> onDisconnect;
		NetStatus status = NetStatus::Init;
        uint16_t Id;
	};
}
