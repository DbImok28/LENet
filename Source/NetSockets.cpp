// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#include "NetSockets.hpp"

namespace LimeEngine::Net
{
	NetSocket::NetSocket(NetSocket&& socket) noexcept : _socket(socket._socket)
	{
		socket._socket = INVALID_SOCKET;
	}

	NetSocket& NetSocket::operator=(NetSocket&& socket) noexcept
	{
		if (&socket != this)
		{
			_socket = socket._socket;
			socket._socket = INVALID_SOCKET;
		}
		return *this;
	}

	NetSocket::NetSocket(NetAddressType addressType, bool async) :
		_socket(WSASocketW(static_cast<int>(addressType), SOCK_STREAM, IPPROTO_TCP, nullptr, 0, async ? WSA_FLAG_OVERLAPPED : 0))
	{
		if (_socket == INVALID_SOCKET) { LENET_LAST_ERROR_MSG("Can't create socket"); }
	}

	NetSocket::~NetSocket()
	{
		Close();
	}

	void NetSocket::SetNonblockingMode(bool isNonblockingMode)
	{
		unsigned long mode = isNonblockingMode;
		if (ioctlsocket(_socket, FIONBIO, &mode) == SOCKET_ERROR) { LENET_LAST_ERROR_MSG("Can't set nonblocking mode"); }
	}

	void NetSocket::SetReuseAddr(bool isReuseAddr)
	{
		const char mode = static_cast<char>(isReuseAddr);
		if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &mode, sizeof(mode)) < 0) { LENET_LAST_ERROR_MSG("Can't set reuse addr mode"); }

#ifdef SO_REUSEPORT
		if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &mode, sizeof(mode)) < 0) { LENET_LAST_ERROR_MSG("Can't set reuse port mode"); }
#endif
	}

	void NetSocket::Bind(NetSocketIPv4Address address)
	{
		if (bind(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) { LENET_LAST_ERROR_MSG("Can't bind socket"); }
	}

	void NetSocket::Listen()
	{
		if (listen(_socket, SOMAXCONN) == SOCKET_ERROR) { LENET_LAST_ERROR_MSG("Can't start to listen"); }
	}

	void NetSocket::Listen(int maxClient) const
	{
		if (listen(_socket, SOMAXCONN_HINT(maxClient)) == SOCKET_ERROR) { LENET_LAST_ERROR_MSG("Can't start to listen"); }
	}

	bool NetSocket::Accept(NetSocket& outSocket) const
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
		NetLogger::LogCore("Accept {}", clientSocket);
		outSocket.SetSocket(clientSocket);
		return true;
	}

	bool NetSocket::Connect(NetSocketIPv4Address address) const
	{
		if (connect(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
		{
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK) LENET_ERROR(err, "Can't connect to server");
			return false;
		}
		return true;
	}

	void NetSocket::Close()
	{
		if (_socket != INVALID_SOCKET)
		{
			NetLogger::LogCore("Close socket {}", _socket);
			closesocket(_socket);
			_socket = INVALID_SOCKET;
		}
	}

	void NetSocket::Shutdown()
	{
		shutdown(_socket, SD_BOTH);
	}

	bool NetSocket::Send(const char* buf, int bufSize, int& outBytesTransferred) const
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

	bool NetSocket::SendAsync(NetBuffer* netBuffer, NativeIOContext* nativeIoContext)
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

	bool NetSocket::Receive(char* buf, int bufSize, int& outBytesTransferred) const
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

	bool NetSocket::ReceiveAsync(NetBuffer* netBuffer, NativeIOContext* nativeIoContext)
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

	NativeSocket NetSocket::GetNativeSocket() const
	{
		return _socket;
	}

	void NetSocket::SetSocket(NativeSocket socket)
	{
		if (IsValid()) Close();
		_socket = socket;
	}

	uint64_t NetSocket::GetId() const
	{
		return _socket;
	}

	bool NetSocket::IsValid() const
	{
		return _socket != InvalidNativeSocket;
	}
}