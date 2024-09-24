#pragma once
#include "NetAddress.hpp"
#include "BufferPool.hpp"

namespace LimeEngine::Net
{
	enum class NetStatus
	{
		Init,
		Error,
		Opened,
		MarkForClose,
		Closed
	};

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
		explicit NetSocket(NetAddressType addressType, bool async = false)
			//: _socket(::WSASocket(static_cast<int>(addressType), SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED))
			:
			_socket(WSASocket(static_cast<int>(addressType), SOCK_STREAM, IPPROTO_TCP, nullptr, 0, async ? WSA_FLAG_OVERLAPPED : 0))
		{
			if (_socket == INVALID_SOCKET) { LENET_ERROR(WSAGetLastError(), "Can't create socket"); }
		}
		//        NetSocket(NetAddressType addressType)
		//                :
		//                _socket(socket(static_cast<int>(addressType), SOCK_STREAM, IPPROTO_TCP))
		//        {
		//            if (_socket == INVALID_SOCKET) { LENET_ERROR(WSAGetLastError(), "Can't create socket"); }
		//        }
		~NetSocket()
		{
			Close();
		}

		void SetNonblockingMode(bool isNonblockingMode = true)
		{
			unsigned long mode = isNonblockingMode;
			if (ioctlsocket(_socket, FIONBIO, &mode) == SOCKET_ERROR) { LENET_ERROR(WSAGetLastError(), "Can't set nonblocking mode"); }
		}
		void SetReuseAddr(bool isReuseAddr = true)
		{
			const char mode = static_cast<char>(isReuseAddr);
			if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &mode, sizeof(mode)) < 0) { LENET_ERROR(WSAGetLastError(), "Can't set reuse addr mode"); }

#ifdef SO_REUSEPORT
			if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &mode, sizeof(mode)) < 0) { LENET_ERROR(WSAGetLastError(), "Can't set reuse port mode"); }
#endif
		}

		void Bind(NetSocketIPv4Address address)
		{
			if (bind(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) { LENET_ERROR(WSAGetLastError(), "Can't bind socket"); }
		}
		void Listen()
		{
			if (listen(_socket, SOMAXCONN) == SOCKET_ERROR) { LENET_ERROR(WSAGetLastError(), "Can't start to listen"); }
		}
		void Listen(int maxClient)
		{
			if (listen(_socket, SOMAXCONN_HINT(maxClient)) == SOCKET_ERROR) { LENET_ERROR(WSAGetLastError(), "Can't start to listen"); }
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

		int Send(const std::string& msg, int& outBytesSent)
		{
			return Send(msg.c_str(), msg.length(), outBytesSent);
		}
		bool Send(const char* buff, int buffSize, int& outBytesSent) const
		{
			outBytesSent = send(_socket, buff, buffSize, 0);
			if (outBytesSent == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK) LENET_ERROR(err, "Can't send message to Client");
				//outBytesSent = 0;
				return false;
			}
			if (outBytesSent == 0) return false;
			return true;
		}
		bool Receive(char* buff, int buffSize, int& outBytesReceived) const
		{
			outBytesReceived = recv(_socket, buff, buffSize, 0);
			if (outBytesReceived == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK) LENET_ERROR(err, "Can't receive message from Client");
				//outBytesReceived = 0;
				return false;
			}
			if (outBytesReceived == 0) return false;
			return true;
		}

//        template<size_t BufferSize>
//        bool Receive(BufferPool<BufferSize>& bufferPool, std::string& outMsg) const
//        {
//            int bytesTransferred;
//            char* receiveBuffer = bufferPool.TakeBuffer();
//            bool received = Receive(receiveBuffer, BufferSize, bytesTransferred);
//            if (!received)
//            {
//                bufferPool.ReturnBuffer(receiveBuffer);
//                return false;
//            }
//
//            if (receiveBuffer[bytesTransferred - 1] == '\0')
//            {
//                outMsg = receiveBuffer;
//                bufferPool.ReturnBuffer(receiveBuffer);
//                return true;
//            }
//
//            std::list<char*> buffers;
//            buffers.push_back(receiveBuffer);
//
//            while (true)
//            {
//                receiveBuffer = bufferPool.TakeBuffer();
//                received = Receive(receiveBuffer, BufferSize, bytesTransferred);
//                if (!received)
//                {
//                    bufferPool.ReturnBuffer(receiveBuffer);
//                    for (auto &buffer: buffers)
//                    {
//                        bufferPool.ReturnBuffer(buffer);
//                    }
//                    return false;
//                }
//
//                buffers.push_back(receiveBuffer);
//
//                if (receiveBuffer[bytesTransferred - 1] == '\0')
//                {
//                    std::ostringstream oss;
//                    for (auto &buffer: buffers)
//                    {
//                        oss << buffer;
//                    }
//                    std::cout << "[msg end]" << std::endl;
//                    for (auto &buffer: buffers)
//                    {
//                        bufferPool.ReturnBuffer(buffer);
//                    }
//                    outMsg = oss.str();
//                    return true;
//                }
//            }
//        }

        template<size_t BufferSize>
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
                    outMsg = bufferChain.ConcatBuffers();
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

		SOCKET GetSocket() const
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
