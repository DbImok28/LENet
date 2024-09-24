#pragma once
#include <queue>
#include <functional>
#include "NetSockets.hpp"

namespace LimeEngine::Net
{
//	class NetSendMessage
//	{
//	public:
//		NetSendMessage(const std::string& msg) : msg(msg), forAll(true) {}
//		NetSendMessage(const std::string& msg, uint16_t clientId) : msg(msg), forAll(false), clientId(clientId) {}
//
//	private:
//		std::string msg;
//		bool forAll;
//		uint16_t clientId;
//	};

	class NetSendMessage
	{
	public:
        NetSendMessage(std::string&& msg) : msg(std::move(msg)) {}
        NetSendMessage(const std::string& msg) : msg(msg) {}

    public:
        std::string msg;
        size_t bytesTransferred = 0;
        bool sended = false;
	};

    class NetReceivedMessage
    {
    public:
        NetReceivedMessage(std::string&& msg) : msg(std::move(msg)) {}
        NetReceivedMessage(const std::string& msg) : msg(msg) {}

    public:
        std::string msg;
        size_t bytesTransferred = 0;
    };

    class NetConnectionOld
    {
    public:
        NetConnectionOld()
        {
            static uint16_t idCounter = 0;
            Id = idCounter++;
        }
        // NetConnection(uint16_t clientId) : clientId(clientId) {}

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
                return false;
            }
            return true;
        }

        void OnMessage(const std::function<void(const NetConnectionOld&, const std::string&)>& handler)
        {
            onMessage = handler;
        }
        void OnDisconnect(const std::function<void(const NetConnectionOld&)>& handler)
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

        std::queue<std::string> messagesToSend;
        std::queue<std::string> receivedMessages;

    private:
        std::function<void(const NetConnectionOld&, const std::string&)> onMessage;
        std::function<void(const NetConnectionOld&)> onDisconnect;
        NetStatus status = NetStatus::Init;
        uint16_t Id;
    };

	class NetConnection
	{
	public:
		NetConnection()
        {
            static uint16_t idCounter = 0;
            Id = idCounter++;
        }
		// NetConnection(uint16_t clientId) : clientId(clientId) {}
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
            std::cout << "[Poll] ";
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
            std::cout << " ";
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

//    class NetTCPPollDataServer
//    {
//    public:
//        NetTCPPollDataServer() noexcept = default;
////        NetTCPListenerServer(const NetTCPListenerServer&) = delete;
////        NetTCPListenerServer& operator=(const NetTCPListenerServer&) = delete;
//
//        bool Receive(NetSocket& socket, NetConnection& connection)
//        {
//            std::array<char, 256> buff;
//            int bytesReceived;
//            if (socket.Receive(buff.data(), buff.size(), bytesReceived))
//            {
//                buff[bytesReceived] = '\0';
//                std::string msg = buff.data();
//                connection.receivedMessages.emplace(msg);
//                std::cout << "Receive(" << bytesReceived << "b" << ((bytesReceived != (msg.size() + 1)) ? " part" : "") << ")" << std::endl;
//            }
//            return bytesReceived != 0;
//        }
//
//        bool Send(NetSocket& socket, NetConnection& connection)
//        {
//            int bytesSent;
//            auto& msg = connection.messagesToSend.front();
//            if (socket.Send(msg.c_str(), msg.size() + 1, bytesSent))
//            {
//                connection.messagesToSend.pop();
//                std::cout << "Send(" << bytesSent << "b" << ((bytesSent != (msg.size() + 1)) ? " part" : "") << ")" << std::endl;
//            }
//            return bytesSent != 0;
//        }
//
//        bool Poll(int timeout = 1)
//        {
//            if (pollFDs.Empty()) return true;
//
//            UpdateWriteFlags();
//
//            int pollResult = pollFDs.WaitForEvents(timeout);
//            if (pollResult == 0) return true;
//
//            pollFDs.Log();
//
//            for (int i = 0; i < pollFDs.Count(); ++i)
//            {
//                if (pollResult == 0) break;
//                --pollResult;
//
//                auto& connection = activeConnections[i].get();
//                auto&& pollFD = pollFDs.At(i);
//
//                //  Read
//                if (pollFD.CheckRead())
//                {
//                    if (!Receive(pollFD.fd, connection))
//                    {
//                        std::cout << "[Net]:[Receive=0] Client disconnected: " << pollFD.fd.GetId() << std::endl;
//                        Disconnect(i);
//                        continue;
//                    }
//                }
//
//                // Write
//                if (pollFD.CheckWrite() && !connection.messagesToSend.empty())
//                {
//                    if (!Send(pollFD.fd, connection))
//                    {
//                        std::cout << "[Net]:[Send=0] Client disconnected: " << pollFD.fd.GetId() << std::endl;
//                        Disconnect(i);
//                        continue;
//                    }
//                    if (connection.messagesToSend.empty())
//                    {
//                        pollFDs.ResetWriteFlag(i);
//                    }
//                }
//
//                // Disconnections
//                if (pollFD.CheckDisconnect())
//                {
//                    std::cout << "[Net] Client disconnected: " << pollFD.fd.GetId() << std::endl;
//                    Disconnect(i);
//                    continue;
//                }
//
//                // Exceptions
//                if (pollFD.CheckExcept())
//                {
//                    LENET_ERROR(WSAGetLastError(), "Unable to read from client");
//                    Disconnect(i);
//                    continue;
//                }
//
//                ++i;
//            }
//            return true;
//        }
//
//    private:
//        void UpdateWriteFlags()
//        {
//            for (size_t i = 0; i < pollFDs.Count(); ++i)
//            {
//                if (!activeConnections[i].get().messagesToSend.empty())
//                {
//                    pollFDs.SetWriteFlag(i);
//                }
//            }
//        }
//
//    public:
//        // Thread 2
//        void AddConnection(NetSocket&& socket, NetConnection& connection)
//        {
//            pollFDs.AddSocket(std::move(socket));
//            activeConnections.emplace_back(connection);
//        }
//
//    private:
////        void Disconnect(std::vector<PollFD>::iterator& socketIter, std::vector<std::reference_wrapper<NetConnection>>::iterator& connectionIter)
////        {
////            pollFDs.RemoveSocket(socketIter);
////            socketIter = pollFDs.erase(socketIter);
////            connectionIter->get().ChangeStateToClose();
////            connectionIter = activeConnections.erase(connectionIter);
////        }
//        void Disconnect(size_t index)
//        {
//            pollFDs.RemoveSocket(index);
//            activeConnections[index].get().ChangeStateToClose();
//            activeConnections.erase(std::begin(activeConnections) + index);
//        }
//
//    private:
//        std::vector<std::reference_wrapper<NetConnection>> activeConnections;
//        PollFDs pollFDs;
//    };
//
//    class NetTCPPollAcceptServer
//    {
//    public:
//        explicit NetTCPPollAcceptServer(NetSocketIPv4Address address) : pollFD(NetSocket(NetAddressType::IPv4), POLLRDNORM)
//        {
//            pollFD.fd.SetNonblockingMode();
//            pollFD.fd.Bind(address);
//            pollFD.fd.Listen();
//
//            dataServers.emplace_back();
//        }
//
//        void Update()
//        {
//            for (auto connectionIter = connections.begin(); connectionIter != connections.end(); )
//            {
//                if (!connectionIter->Update()) connectionIter = connections.erase(connectionIter);
//                else ++connectionIter;
//            }
//        }
//
//        bool PollAccept(int timeout = 0)
//        {
//            int pollResult = WSAPoll(reinterpret_cast<WSAPOLLFD*>(&pollFD), 1, timeout);
//            if (pollResult < 0)
//            {
//                LENET_ERROR(WSAGetLastError(), "Can't to poll");
//                return false;
//            }
//            if (pollResult == 0) return true;
//
//            if (pollFD.CheckRead())
//            {
//                NetSocket clientSocket;
//                if (pollFD.fd.Accept(clientSocket))
//                {
//                    AddConnection(std::move(clientSocket));
//                }
//            }
//            return true;
//        }
//
//        void PollListeners()
//        {
//            for (auto& server : dataServers)
//            {
//                server.Poll();
//            }
//        }
//
//        void OnConnection(const std::function<void(NetConnection&)>& handler)
//        {
//            onConnection = handler;
//        }
//
//    private:
//        void AddConnection(NetSocket&& socket)
//        {
//            connections.emplace_back();
//            auto& connection = connections.back();
//            GetAvailableDataServer().AddConnection(std::move(socket), connection);
//            onConnection(connection);
//        }
//        NetTCPPollDataServer& GetAvailableDataServer()
//        {
//            auto& availableServer = dataServers[availableServerIndex];
//            availableServerIndex = (availableServerIndex + 1ull) % dataServers.size();
//            return availableServer;
//        }
//
//    public:
//    //private:
//        PollFD pollFD;
//        std::list<NetConnection> connections;
//        std::vector<NetTCPPollDataServer> dataServers;
//        size_t availableServerIndex = 0ull;
//
//        std::function<void(NetConnection&)> onConnection;
//    };
}
