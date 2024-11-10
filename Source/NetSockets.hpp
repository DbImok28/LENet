#pragma once
#include "NetAddress.hpp"
#include "BufferPool.hpp"

namespace LimeEngine::Net
{
	class NetSocket
	{
	public:
		NetSocket(const NetSocket&) = delete;
		NetSocket& operator=(const NetSocket&) = delete;

		NetSocket(NetSocket&& socket) noexcept;
		NetSocket& operator=(NetSocket&& socket) noexcept;

		NetSocket() noexcept = default;
		explicit NetSocket(SOCKET _socket) noexcept : _socket(_socket) {}
		explicit NetSocket(NetAddressType addressType, bool async = false);

		~NetSocket();

	public:
		void SetNonblockingMode(bool isNonblockingMode = true);
		void SetReuseAddr(bool isReuseAddr = true);

		void Bind(NetSocketIPv4Address address);
		void Listen();
		void Listen(int maxClient) const;
		bool Accept(NetSocket& outSocket) const;
		bool Connect(NetSocketIPv4Address address) const;

		void Close();
		void Shutdown();

		bool Send(const char* buf, int bufSize, int& outBytesTransferred) const;
		bool SendAsync(NetBuffer* netBuffer, NativeIOContext* nativeIoContext);

		bool Receive(char* buf, int bufSize, int& outBytesTransferred) const;
		bool ReceiveAsync(NetBuffer* netBuffer, NativeIOContext* nativeIoContext);

		template <size_t BufferSize>
		bool Receive(BufferPool<BufferSize>& bufferPool, std::string& outMsg) const
		{
			BufferChain<BufferSize> bufferChain(bufferPool);
			int bytesTransferred;

			char* receiveBuffer = bufferChain.TakeBuffer();
			if (!Receive(receiveBuffer, BufferSize, bytesTransferred)) return false;
			if (receiveBuffer[bytesTransferred - 1] == '\0')
			{
				outMsg = receiveBuffer;
				return true;
			}

			while (true)
			{
				receiveBuffer = bufferPool.TakeBuffer();
				if (!Receive(receiveBuffer, BufferSize, bytesTransferred)) return false;
				if (receiveBuffer[bytesTransferred - 1] == '\0')
				{
					outMsg = bufferChain.Concat();
					return true;
				}
			}
		}

		NativeSocket GetNativeSocket() const;
		void SetSocket(NativeSocket socket);

		uint64_t GetId() const;
		bool IsValid() const;

	protected:
		NativeSocket _socket = INVALID_SOCKET;
	};
}
