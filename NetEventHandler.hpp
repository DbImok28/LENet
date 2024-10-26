#pragma once
#include "NetConnection.hpp"
#include "NetSockets.hpp"
#include "BufferPool.hpp"

namespace LimeEngine::Net
{
    enum class IOOperationType
    {
        Send,
        Receive
    };

    struct IOContext
    {
        explicit IOContext(IOOperationType operationType) noexcept : operationType(operationType), wsaBuf(0, nullptr)
        {
            overlapped.Internal = 0;
            overlapped.InternalHigh = 0;
            overlapped.Offset = 0;
            overlapped.OffsetHigh = 0;
            overlapped.hEvent = nullptr;
        }
        ~IOContext() noexcept
        {
            std::cout << "~IOContext()" << std::endl;
        }

        void SetNextBuffer(const char* buffer)
        {
            SetNextBuffer(const_cast<char*>(buffer));
        }
        void SetNextBuffer(char* buffer)
        {
            wsaBuf.buf = buffer;
            buffers.emplace_back(buffer);
        }

        std::pair<char*, uint32_t> GetBuffer()
        {
            return std::make_pair(wsaBuf.buf, wsaBuf.len);
        }
        const std::list<char*>& GetBuffers()
        {
            return buffers;
        }
        void SetMessageLength(uint32_t messageLen)
        {
            wsaBuf.len = messageLen;
        }
        void Reset()
        {
            buffers.clear();
        }

        static IOContext* FromOverlapped(LPOVERLAPPED overlapped) noexcept
        {
            return CONTAINING_RECORD(overlapped, IOContext, overlapped);
        }

    public:
        OVERLAPPED overlapped;

    public:
        WSABUF wsaBuf;
        std::list<char*> buffers;
        IOOperationType operationType = IOOperationType::Receive;
    };

    struct SocketContext
    {
        SocketContext(NetSocket&& socket, NetConnection* connection) : socket(std::move(socket)), connection(connection) {}
        ~SocketContext() noexcept
        {
            std::cout << "~SocketContext()" << std::endl;
        }

        NetSocket socket;
        NetConnection* connection;
        IOContext receiveContext{ IOOperationType::Receive };
        IOContext sendContext{ IOOperationType::Send };
    };


    class NetEventHandler
    {
    public:
        void StartRead(SocketContext& socketContext)
        {
            socketContext.receiveContext.SetMessageLength(bufferPool.size);
            socketContext.receiveContext.SetNextBuffer(bufferPool.TakeBuffer());
        }
        void Read(SocketContext& socketContext, uint32_t bytesTransferred)
        {
            IOContext& ioContext = socketContext.receiveContext;
            auto [buffer, size] = ioContext.GetBuffer();

            NetLogger::LogCore("Read: {}", buffer);

            if (buffer[bytesTransferred - 1] == '\0')
            {
                NetReceivedMessage fullMsg = NetReceivedMessage(ConcatBuffers(ioContext.GetBuffers(), bufferPool.size));
                socketContext.connection->receivedMessages.emplace(std::move(fullMsg));

                NetLogger::LogCore("[msg end]");

                bufferPool.ReturnBuffers(ioContext.GetBuffers());
                ioContext.Reset();
            }
            ioContext.SetNextBuffer(bufferPool.TakeBuffer());
        }

        bool StartWrite(SocketContext& socketContext)
        {
            if (!socketContext.connection->messagesToSend.empty())
            {
                auto& sendMsg = socketContext.connection->messagesToSend.back();
                if (sendMsg.sended) return false;

                socketContext.sendContext.SetMessageLength(sendMsg.msg.length() + 1);
                socketContext.sendContext.SetNextBuffer(sendMsg.msg.c_str());
                sendMsg.sended = true;
                return true;
            }
            return false;
        }
        bool Write(SocketContext& socketContext, uint32_t bytesTransferred)
        {
            socketContext.connection->messagesToSend.pop();
            socketContext.sendContext.Reset();
            return StartWrite(socketContext);
        }
        bool ReadyToWrite(SocketContext& socketContext)
        {
            NetLogger::LogCore("ReadyToWrite {}", !socketContext.connection->messagesToSend.empty());
            return !socketContext.connection->messagesToSend.empty();
        }

        bool Disconnect(SocketContext& socketContext)
        {
            socketContext.connection->ChangeStateToClose();
            return true;
        }

    private:
        BufferPool<1024> bufferPool;
    };
}
