#pragma once
#include "../NetBase.hpp"

namespace LimeEngine::Net
{
	class NetSocket;
	class IOContext;

	class NetProtocolTCP
	{
	public:
		static bool Send(NetSocket& socket, const char* buf, int bufSize, int& outBytesTransferred);
		static bool SendAsync(NetSocket& socket, NetBuffer* netBuffer, NativeIOContext* nativeIoContext);

		static bool Receive(NetSocket& socket, char* buf, int bufSize, int& outBytesTransferred);
		static bool ReceiveAsync(NetSocket& socket, NetBuffer* netBuffer, NativeIOContext* nativeIoContext);
	};
}
