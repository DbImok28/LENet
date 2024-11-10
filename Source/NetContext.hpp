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
		explicit IOContext(IOOperationType operationType) noexcept;

		void SetNextBuffer(const char* buffer);
		void SetNextBuffer(char* buffer);
		void SetMessageLength(uint32_t messageLen);
		void Reset();

		std::pair<char*, uint32_t> GetBuffer() const;
		const std::list<char*>& GetBuffers() const;

		static IOContext* FromNativeIoContext(NativeIOContext* nativeIoContext) noexcept;

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
		SocketContext(NetSocket&& socket, NetConnection* connection) : socket(std::move(socket)), connection(connection) {}

		NetSocket socket;
		NetConnection* connection;
		IOContext receiveContext{ IOOperationType::Receive };
		IOContext sendContext{ IOOperationType::Send };
	};
}