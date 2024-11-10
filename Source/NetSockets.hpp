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

		NetSocket(NetSocket&& socket) noexcept : _socket(socket._socket)
		{
			socket._socket = INVALID_SOCKET;
		}
		NetSocket& operator=(NetSocket&& socket) noexcept
		{
			if (&socket != this)
			{
				_socket = socket._socket;
				socket._socket = INVALID_SOCKET;
			}
			return *this;
		}

		NetSocket() noexcept = default;
		explicit NetSocket(SOCKET _socket) noexcept : _socket(_socket) {}
		explicit NetSocket(NetAddressType addressType, bool async = false) :
			_socket(WSASocketW(static_cast<int>(addressType), SOCK_STREAM, IPPROTO_TCP, nullptr, 0, async ? WSA_FLAG_OVERLAPPED : 0))
		{
			if (_socket == INVALID_SOCKET) { LENET_LAST_ERROR_MSG("Can't create socket"); }
		}
		~NetSocket()
		{
			Close();
		}

		void SetNonblockingMode(bool isNonblockingMode = true)
		{
			unsigned long mode = isNonblockingMode;
			if (ioctlsocket(_socket, FIONBIO, &mode) == SOCKET_ERROR) { LENET_LAST_ERROR_MSG("Can't set nonblocking mode"); }
		}
		void SetReuseAddr(bool isReuseAddr = true)
		{
			const char mode = static_cast<char>(isReuseAddr);
			if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &mode, sizeof(mode)) < 0) { LENET_LAST_ERROR_MSG("Can't set reuse addr mode"); }

#ifdef SO_REUSEPORT
			if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &mode, sizeof(mode)) < 0) { LENET_LAST_ERROR_MSG("Can't set reuse port mode"); }
#endif
		}

		void Bind(NetSocketIPv4Address address)
		{
			if (bind(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) { LENET_LAST_ERROR_MSG("Can't bind socket"); }
		}
		void Listen()
		{
			if (listen(_socket, SOMAXCONN) == SOCKET_ERROR) { LENET_LAST_ERROR_MSG("Can't start to listen"); }
		}
		void Listen(int maxClient)
		{
			if (listen(_socket, SOMAXCONN_HINT(maxClient)) == SOCKET_ERROR) { LENET_LAST_ERROR_MSG("Can't start to listen"); }
		}
		bool Accept(NetSocket& outSocket) const
		{
			sockaddr_in clientSocketAddr{};
			int clientAddrSize = sizeof(clientSocketAddr);

			SOCKET clientSocket = WSAAccept(_socket, reinterpret_cast<sockaddr*>(&clientSocketAddr), &clientAddrSize, nullptr, 0);
			if (clientSocket == INVALID_SOCKET)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK) LENET_ERROR(err, "Can't accept connection");
				return false;
			}
			std::cout << "Accept " << (HANDLE)clientSocket << std::endl;
			outSocket.SetSocket(clientSocket);
			return true;
		}
		bool Connect(NetSocketIPv4Address address) const
		{
			if (connect(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK) LENET_ERROR(err, "Can't connect to server");
				return false;
			}
			return true;
		}

		bool Send(const char* buf, int bufSize, int& outBytesTransferred) const
		{
			outBytesTransferred = send(_socket, buf, bufSize, 0);
			if (outBytesTransferred == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err == WSAECONNRESET || err == WSAEWOULDBLOCK)
				{
					outBytesTransferred = 0;
					return false;
				}
				LENET_ERROR(err, "Can't send message to Client");
				return false;
			}
			if (outBytesTransferred == 0) return false;
			return true;
		}
		bool SendAsync(NetBuffer* netBuffer, NativeIOContext* nativeIoContext)
		{
			DWORD flags = 0;
			if (WSASend(_socket, netBuffer, 1, nullptr, flags, nativeIoContext, nullptr) == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err == WSA_IO_PENDING) return true;
				LENET_ERROR(err, "Can't to send async message to Client");
                return false;
            }
            return true;
		}

		bool Receive(char* buf, int bufSize, int& outBytesTransferred) const
		{
			outBytesTransferred = recv(_socket, buf, bufSize, 0);
			if (outBytesTransferred == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err == WSAECONNRESET || err == WSAEWOULDBLOCK)
				{
					outBytesTransferred = 0;
					return false;
				}
				LENET_ERROR(err, "Can't receive message from Client");
				return false;
			}
			if (outBytesTransferred == 0) return false;
			return true;
		}
		bool ReceiveAsync(NetBuffer* netBuffer, NativeIOContext* nativeIoContext)
		{
			DWORD flags = 0;
			if (WSARecv(_socket, netBuffer, 1, nullptr, &flags, nativeIoContext, nullptr) == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err == WSA_IO_PENDING) return true;
				LENET_ERROR(err, "Can't to receive async message from Client");
                return false;
			}
            return true;
		}

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

		void Close()
		{
			if (_socket != INVALID_SOCKET)
			{
				std::cout << "Close socket: " << _socket << std::endl;
				closesocket(_socket);
				_socket = INVALID_SOCKET;
			}
		}

		void Shutdown()
		{
			shutdown(_socket, SD_BOTH);
		}

		SOCKET GetNativeSocket() const
		{
			return _socket;
		}

		void SetSocket(SOCKET socket)
		{
			if (IsValid()) Close();
			_socket = socket;
		}

		uint64_t GetId() const
		{
			return _socket;
		}

		bool IsValid() const
		{
			return _socket != INVALID_SOCKET;
		}

	protected:
		SOCKET _socket = INVALID_SOCKET;
	};
}
