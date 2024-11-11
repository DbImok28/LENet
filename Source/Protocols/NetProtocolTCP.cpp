// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#include "NetProtocolTCP.hpp"
#include "../NetContext.hpp"

namespace LimeEngine::Net
{
	bool NetProtocolTCP::Send(NetSocket& socket, const char* buf, int bufSize, int& outBytesTransferred)
	{
		if (socket.Send(buf, bufSize, outBytesTransferred))
		{
			NetLogger::LogCore("Send {}b{}: {}", outBytesTransferred, ((outBytesTransferred != (bufSize + 1ull)) ? " part" : ""), buf);
			return true;
		}
		return false;
	}

	bool NetProtocolTCP::SendAsync(NetSocket& socket, NetBuffer* netBuffer, NativeIOContext* nativeIoContext)
	{
		if (socket.SendAsync(netBuffer, nativeIoContext))
		{
			NetLogger::LogCore("Async Send started");
			return true;
		}
		return false;
	}

	bool NetProtocolTCP::Receive(NetSocket& socket, char* buf, int bufSize, int& outBytesTransferred)
	{
		if (socket.Receive(buf, bufSize, outBytesTransferred))
		{
			NetLogger::LogCore("Receive {}b: {}", outBytesTransferred, buf);
			return true;
		}
		return false;
	}

	bool NetProtocolTCP::ReceiveAsync(NetSocket& socket, NetBuffer* netBuffer, NativeIOContext* nativeIoContext)
	{
		if (socket.ReceiveAsync(netBuffer, nativeIoContext))
		{
			NetLogger::LogCore("Async Receive started");
			return true;
		}
		return false;
	}
}
