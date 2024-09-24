#pragma once
#include <queue>
#include <functional>
#include "NetPollServer.hpp"

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
            std::cout << "[Select] ";
            for (auto& socket : sockets)
            {
                std::cout << '[';
                std::cout << ((FD_ISSET(socket.GetSocket(), &readFDs)) ? "R" : "");
                std::cout << ((FD_ISSET(socket.GetSocket(), &writeFDs)) ? "W" : "");
                std::cout << ((FD_ISSET(socket.GetSocket(), &exceptFDs)) ? "E" : "");
                std::cout << ']';
            }
            std::cout << " ";
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

//    class NetTCPSelectDataServer
//    {
//    public:
//        NetTCPSelectDataServer() noexcept = default;
////        NetTCPListenerServer(const NetTCPListenerServer&) = delete;
////        NetTCPListenerServer& operator=(const NetTCPListenerServer&) = delete;
//
//        static bool Receive(NetSocket& socket, NetConnection& connection)
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
//        static bool Send(NetSocket& socket, NetConnection& connection)
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
//        bool Select(int timeout = 1)
//        {
//            if (selectFDs.Empty()) return true;
//
//            UpdateWriteFlags();
//
//            int selectResult = selectFDs.WaitForEvents(timeout);
//            if (selectResult == 0) return true;
//
//            selectFDs.Log();
//
//            for (int i = 0; i < selectFDs.Count();)
//            {
//                if (selectResult == 0) break;
//                --selectResult;
//
//                auto& connection = activeConnections[i].get();
//                auto&& selectFD = selectFDs.At(i);
//
//                //  Read
//                if (selectFD.CheckRead())
//                {
//                    if (!Receive(selectFD.fd, connection))
//                    {
//                        std::cout << "[Net]:[Receive=0] Client disconnected: " << selectFD.fd.GetId() << std::endl;
//                        Disconnect(i);
//                        continue;
//                    }
//                }
//
//                // Write
//                if (selectFD.CheckWrite() && !connection.messagesToSend.empty())
//                {
//                    if (!Send(selectFD.fd, connection))
//                    {
//                        std::cout << "[Net]:[Send=0] Client disconnected: " << selectFD.fd.GetId() << std::endl;
//                        Disconnect(i);
//                        continue;
//                    }
//                    if (connection.messagesToSend.empty())
//                    {
//                        selectFDs.ResetWriteFlag(i);
//                    }
//                }
//
//                // Exceptions
//                if (selectFD.CheckExcept())
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
//    private:
//        void UpdateWriteFlags()
//        {
//            for (size_t i = 0; i < selectFDs.Count(); ++i)
//            {
//                if (!activeConnections[i].get().messagesToSend.empty())
//                {
//                    selectFDs.SetWriteFlag(i);
//                }
//            }
//        }
//
//    public:
//        // Thread 2
//        void AddConnection(NetSocket&& socket, NetConnection& connection)
//        {
//            selectFDs.AddSocket(std::move(socket));
//            activeConnections.emplace_back(connection);
//        }
//
//    private:
//        void Disconnect(size_t index)
//        {
//            selectFDs.RemoveSocket(index);
//            activeConnections[index].get().ChangeStateToClose();
//            activeConnections.erase(std::begin(activeConnections) + index);
//        }
//
//    private:
//        static constexpr size_t maxConnections = FD_SETSIZE;
//
//        std::vector<std::reference_wrapper<NetConnection>> activeConnections;
//        SelectFDs selectFDs;
//    };

//    class NetTCPSelectDataServer
//    {
//    public:
//        NetTCPSelectDataServer() noexcept
//        {
//            FD_ZERO(&readFDs);
//            FD_ZERO(&writeFDs);
//            FD_ZERO(&exceptFDs);
//        }
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
//        bool Select(int timeout = 1)
//        {
//            if (sockets.empty()) return true;
//
//            fd_set readFDsCopy = readFDs;
//            fd_set writeFDsCopy = writeFDs;
//            fd_set exceptFDsCopy = exceptFDs;
//
//            std::cout << "Select()";
//            timeval tv;
//            tv.tv_sec = 1;
//            tv.tv_usec = 0;
//            //int selectResult = select(FD_SETSIZE, &readySockets, nullptr, nullptr, nullptr);
//            int selectResult = select(largestSocket, &readFDsCopy, &writeFDsCopy, &exceptFDsCopy, &tv);
//            if (selectResult < 0)
//            {
//                // EWOULDBLOCK
//                LENET_ERROR(WSAGetLastError(), "Can't to select");
//                return false;
//            }
//            if (selectResult == 0) return true;
//
//            //LogSelect(pollFDs);
//            for (int i = 0; i < FD_SETSIZE; ++i)
//            {
//                if (selectResult == 0) break;
//                --selectResult;
//
//                auto& connection = activeConnections[i].get();
//                auto& socket = sockets[i];
//                auto fd = socket.GetSocket();
//
//                std::cout << '[';
//                std::cout << ((FD_ISSET(fd, &readFDsCopy)) ? "R" : "");
//                std::cout << ((FD_ISSET(fd, &writeFDsCopy)) ? "W" : "");
//                std::cout << ((FD_ISSET(fd, &exceptFDsCopy)) ? "E" : "");
//                std::cout << ']';
//                std::cout << std::endl;
//                //  Read
//                if (FD_ISSET(fd, &readFDsCopy))
//                {
//                    if (!Receive(socket, connection))
//                    {
//                        std::cout << "[Net]:[Receive=0] Client disconnected: " << socket.GetId() << std::endl;
//                        Disconnect(i);
//                        continue;
//                    }
//                }
//
//                // Write
//                if (FD_ISSET(fd, &writeFDsCopy) && !connection.messagesToSend.empty())
//                {
//                    if (!Send(socket, connection))
//                    {
//                        std::cout << "[Net]:[Send=0] Client disconnected: " << socket.GetId() << std::endl;
//                        Disconnect(i);
//                        continue;
//                    }
//                }
//
//                // Exceptions
//                if (FD_ISSET(fd, &exceptFDsCopy))
//                {
//                    LENET_ERROR(WSAGetLastError(), "Unable to read from client");
//                    Disconnect(i);
//                    continue;
//                }
//            }
//            return true;
//        }
//
//    public:
//        // Thread 2
//        void AddConnection(NetSocket&& socket, NetConnection& connection)
//        {
//            SOCKET fd = socket.GetSocket();
//            FD_SET(fd, &readFDs);
//            FD_SET(fd, &writeFDs);
//            FD_SET(fd, &exceptFDs);
//            if (fd > largestSocket)
//            {
//                largestSocket = fd;
//            }
//
//            sockets.emplace_back(std::move(socket));
//            activeConnections.emplace_back(connection);
//        }
//
//    private:
//        void Disconnect(size_t index)
//        {
//            SOCKET fd = sockets[index].GetSocket();
//            FD_CLR(fd, &readFDs);
//            FD_CLR(fd, &writeFDs);
//            FD_CLR(fd, &exceptFDs);
//            if (largestSocket == fd)
//            {
//                auto& fds = *reinterpret_cast<std::vector<SOCKET>*>(&sockets);
//                largestSocket = *std::max_element(std::begin(fds), std::end(fds));
//                //largestSocket = std::max_element(std::begin(sockets), std::end(sockets), [](auto& socket){ return socket.GetSocket();})->GetSocket();
//            }
//
//            sockets.erase(std::begin(sockets) + index);
//            activeConnections[index].get().ChangeStateToClose();
//            activeConnections.erase(std::begin(activeConnections) + index);
//        }
//
//    private:
//        static constexpr size_t maxConnections = FD_SETSIZE;
//
//        std::vector<std::reference_wrapper<NetConnection>> activeConnections;
//        std::vector<NetSocket> sockets;
//        fd_set readFDs;
//        fd_set writeFDs;
//        fd_set exceptFDs;
//        int largestSocket;
//    };

//    class NetTCPSelectAcceptServer
//    {
//    public:
//        explicit NetTCPSelectAcceptServer(NetSocketIPv4Address address) : pollFD(NetSocket(NetAddressType::IPv4))
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
//        void SelectListeners()
//        {
//            for (auto& server : dataServers)
//            {
//                server.Select();
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
//        NetTCPSelectDataServer& GetAvailableDataServer()
//        {
//            auto& availableServer = dataServers[availableServerIndex];
//            availableServerIndex = (availableServerIndex + 1ull) % dataServers.size();
//            return availableServer;
//        }
//
//    public:
//        //private:
//        PollFD pollFD;
//        std::list<NetConnection> connections;
//        std::vector<NetTCPSelectDataServer> dataServers;
//        size_t availableServerIndex = 0ull;
//
//        std::function<void(NetConnection&)> onConnection;
//    };
}
