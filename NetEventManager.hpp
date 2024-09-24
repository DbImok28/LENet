#pragma once
#include <queue>
#include <functional>
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
        static bool Receive(NetSocket& socket, NetConnectionOld& connection)
        {
            std::array<char, 256> buff;
            int bytesReceived;
            if (socket.Receive(buff.data(), buff.size(), bytesReceived))
            {
                buff[bytesReceived] = '\0';
                std::string msg = buff.data();
                connection.receivedMessages.emplace(msg);
                std::cout << "Receive(" << bytesReceived << "b" << ((bytesReceived != (msg.size() + 1)) ? " part" : "") << ")" << std::endl;
            }
            return bytesReceived != 0;
        }

        static bool Send(NetSocket& socket, NetConnectionOld& connection)
        {
            int bytesSent;
            auto& msg = connection.messagesToSend.front();
            if (socket.Send(msg.c_str(), msg.size() + 1, bytesSent))
            {
                connection.messagesToSend.pop();
                std::cout << "Send(" << bytesSent << "b" << ((bytesSent != (msg.size() + 1)) ? " part" : "") << ")" << std::endl;
            }
            return bytesSent != 0;
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
        void AddConnection(NetSocket&& socket, NetConnectionOld& connection)
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

                //  Read
                if (netEvent.CheckRead())
                {
                    if (!TNetDataHandler::Receive(netEvent.fd, connection))
                    {
                        std::cout << "[Net]:[Receive=0] Client disconnected: " << netEvent.fd.GetId() << std::endl;
                        Disconnect(i);
                        continue;
                    }
                }

                // Write
                if (netEvent.CheckWrite() && !connection.messagesToSend.empty())
                {
                    if (!TNetDataHandler::Send(netEvent.fd, connection))
                    {
                        std::cout << "[Net]:[Send=0] Client disconnected: " << netEvent.fd.GetId() << std::endl;
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
                    std::cout << "[Net] Client disconnected: " << netEvent.fd.GetId() << std::endl;
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
        std::vector<std::reference_wrapper<NetConnectionOld>> activeConnections;
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
        void OnConnection(const std::function<void(NetConnectionOld&)>& handler)
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
        std::list<NetConnectionOld> connections;
        std::vector<TNetEventHandler> netEventHandler;
        size_t availableServerIndex = 0ull;

        std::function<void(NetConnectionOld&)> onConnection;
    };
}
