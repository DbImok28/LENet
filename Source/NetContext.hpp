#pragma once
#include "NetConnection.hpp"
#include "NetSockets.hpp"

namespace LimeEngine::Net
{
    enum class IOOperationType
    {
        Send,
        Receive
    };

    class IOContext
    {
    public:
        explicit IOContext(IOOperationType operationType) noexcept: operationType(operationType), netBuffer(0, nullptr)
        {
            nativeIoContext.Internal = 0;
            nativeIoContext.InternalHigh = 0;
            nativeIoContext.Offset = 0;
            nativeIoContext.OffsetHigh = 0;
            nativeIoContext.hEvent = nullptr;
        }

        ~IOContext() noexcept
        {
            std::cout << "~IOContext()" << std::endl;
        }

        void SetNextBuffer(const char *buffer)
        {
            SetNextBuffer(const_cast<char *>(buffer));
        }

        void SetNextBuffer(char *buffer)
        {
            netBuffer.buf = buffer;
            buffers.emplace_back(buffer);
        }

        std::pair<char *, uint32_t> GetBuffer()
        {
            return std::make_pair(netBuffer.buf, netBuffer.len);
        }

        const std::list<char*>& GetBuffers()
        {
            return buffers;
        }

        void SetMessageLength(uint32_t messageLen)
        {
            netBuffer.len = messageLen;
        }

        void Reset()
        {
            buffers.clear();
        }

        static IOContext *FromNativeIoContext(NativeIOContext* nativeIoContext) noexcept
        {
            return CONTAINING_RECORD(nativeIoContext, IOContext, nativeIoContext);
        }

    public:
        NativeIOContext nativeIoContext;

    public:
        NetBuffer netBuffer;
        std::list<char*> buffers;
        IOOperationType operationType = IOOperationType::Receive;
    };

    class SocketContext
    {
    public:
        SocketContext(NetSocket &&socket, NetConnection *connection) : socket(std::move(socket)), connection(connection) {}

        ~SocketContext() noexcept
        {
            std::cout << "~SocketContext()" << std::endl;
        }

        NetSocket socket;
        NetConnection *connection;
        IOContext receiveContext{IOOperationType::Receive};
        IOContext sendContext{IOOperationType::Send};
    };
}