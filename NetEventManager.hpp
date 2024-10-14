#pragma once
#include "NetPollServer.hpp"
#include "NetSelectServer.hpp"

namespace LimeEngine::Net
{
    template<typename TNetEventManager, typename TNetDataHandler>
    class NetBufferBasedEventHandler;

    template<typename TNetDataHandler>
    using NetSelectHandler = NetBufferBasedEventHandler<NetSelectManager, TNetDataHandler>;
    template<typename TNetDataHandler>
    using NetPollHandler = NetBufferBasedEventHandler<NetPollManager, TNetDataHandler>;

    template<typename TNetDataHandler>
    using NetEventHandler = NetBufferBasedEventHandler<NetPollManager, TNetDataHandler>;

    struct NetTCPDataHandler
    {
        static bool Receive(NetSocket& socket, char* buffer, int bufferSize, int& outBytesTransferred)
        {
            if (socket.Receive(buffer, bufferSize, outBytesTransferred))
            {
                NetLogger::LogCore("Receive {}b: {}", outBytesTransferred, buffer);
            }
            return outBytesTransferred != 0;
        }

        static bool Send(NetSocket& socket, const NetSendMessage& sendMessage)
        {
            int bytesTransferred;
            if (socket.Send(sendMessage.msg.c_str(), static_cast<int>(sendMessage.msg.size()) + 1, bytesTransferred))
            {
                NetLogger::LogCore("Send {}b{}: {}", bytesTransferred, ((bytesTransferred != (sendMessage.msg.size() + 1ull)) ? " part" : ""), sendMessage.msg);
            }
            return bytesTransferred != 0;
        }
    };

    template<typename TNetEventHandler, typename TNetDataHandler>
    class NetBufferBasedEventHandler
    {
    public:
        NetBufferBasedEventHandler() noexcept = default;
//        NetBufferBasedEventHandler(const NetTCPListenerServer&) = delete;
//        NetBufferBasedEventHandler& operator=(const NetTCPListenerServer&) = delete;

    public:
        // Thread 2
        void AddConnection(NetSocket&& socket, NetConnection& connection)
        {
            netBufferedEventHandler.AddSocket(std::move(socket));
            activeConnections.emplace_back(connection);
        }

    private:
        void Disconnect(size_t index)
        {
            netBufferedEventHandler.RemoveSocket(index);
            activeConnections[index].get().ChangeStateToClose();
            activeConnections.erase(std::begin(activeConnections) + index);
        }

    public:
        bool HandleNetEvents(uint32_t timeout = 1u)
        {
            if (netBufferedEventHandler.Empty()) return true;

            UpdateWriteFlags();

            int pollResult = netBufferedEventHandler.WaitForEvents(timeout);
            if (pollResult == 0) return true;

            netBufferedEventHandler.Log();

            for (int i = 0; i < netBufferedEventHandler.Count(); ++i)
            {
                if (pollResult == 0) break;
                --pollResult;

                auto& connection = activeConnections[i].get();
                auto&& netEvent = netBufferedEventHandler.At(i);

                // TODO: Add buffering

                //  Read
                if (netEvent.CheckRead())
                {
                    std::array<char, 256> buff;
                    int bytesTransferred;

                    if (TNetDataHandler::Receive(netEvent.fd, buff.data(), buff.size(), bytesTransferred))
                    {
                        connection.receivedMessages.emplace(buff.data());
                    }
                    else
                    {
                        NetLogger::LogCore("Receive=0, Client {} disconnected", netEvent.fd.GetId());
                        Disconnect(i);
                        continue;
                    }
                }

                // Write
                if (netEvent.CheckWrite() && !connection.messagesToSend.empty())
                {
                    if (TNetDataHandler::Send(netEvent.fd, connection.messagesToSend.front()))
                    {
                        connection.messagesToSend.pop();
                    }
                    else
                    {
                        NetLogger::LogCore("Send=0, Client {} disconnected", netEvent.fd.GetId());
                        Disconnect(i);
                        continue;
                    }
                    if (connection.messagesToSend.empty())
                    {
                        netBufferedEventHandler.ResetWriteFlag(i);
                    }
                }

                // Disconnections
                if (netEvent.CheckDisconnect())
                {
                    NetLogger::LogCore("Client {} disconnected", netEvent.fd.GetId());
                    Disconnect(i);
                    continue;
                }

                // Exceptions
                if (netEvent.CheckExcept())
                {
                    LENET_ERROR(WSAGetLastError(), "Unable to read from client");
                    Disconnect(i);
                    continue;
                }

                ++i;
            }
            return true;
        }

    private:
        void UpdateWriteFlags()
        {
            for (size_t i = 0; i < netBufferedEventHandler.Count(); ++i)
            {
                if (!activeConnections[i].get().messagesToSend.empty())
                {
                    netBufferedEventHandler.SetWriteFlag(i);
                }
            }
        }

    private:
        std::vector<std::reference_wrapper<NetConnection>> activeConnections;
        TNetEventHandler netBufferedEventHandler;
    };

    template<typename TNetEventHandler>
    class NetTCPServer
    {
    public:
        explicit NetTCPServer(NetSocketIPv4Address address) : serverSocket(NetAddressType::IPv4)
        {
            serverSocket.SetNonblockingMode();
            serverSocket.Bind(address);
            serverSocket.Listen();

            netEventHandler.emplace_back();
        }
        void Update()
        {
            for (auto connectionIter = connections.begin(); connectionIter != connections.end(); )
            {
                if (!connectionIter->Update()) connectionIter = connections.erase(connectionIter);
                else ++connectionIter;
            }
        }
        void Accept()
        {
            NetSocket clientSocket;
            if (serverSocket.Accept(clientSocket))
            {
                AddConnection(std::move(clientSocket));
            }
        }
        void HandleNetEvents()
        {
            for (auto& handler : netEventHandler)
            {
                handler.HandleNetEvents();
            }
        }
        void OnConnection(const std::function<void(NetConnection&)>& handler)
        {
            onConnection = handler;
        }

    private:
        void AddConnection(NetSocket&& socket)
        {
            connections.emplace_back();
            auto& connection = connections.back();
            GetAvailableEventHandler().AddConnection(std::move(socket), connection);
            onConnection(connection);
        }
        TNetEventHandler& GetAvailableEventHandler()
        {
            auto& availableHandler = netEventHandler[availableServerIndex];
            availableServerIndex = (availableServerIndex + 1ull) % netEventHandler.size();
            return availableHandler;
        }

    public:
    //private:
        NetSocket serverSocket;
        std::list<NetConnection> connections;
        std::vector<TNetEventHandler> netEventHandler;
        size_t availableServerIndex = 0ull;

        std::function<void(NetConnection&)> onConnection;
    };
}
