#pragma once
#define NOMINMAX

#include "PlatformDetection.hpp"

#ifdef LE_BUILD_PLATFORM_WINDOWS
#define FD_SETSIZE 1024
#endif

#include <iostream>
#include <array>
#include <vector>
#include <functional>
#include <format>
#include <chrono>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "WinSocketError.hpp"
#include "NetLogger.hpp"

#define LENET_ERROR(err, msg)                                            \
	{                                                                    \
		int _err = err;                                                  \
        ::LimeEngine::Net::NetLogger::LogCore("{}:{} Error: code {}({}), {}\n desc: {}", __FILE__, __LINE__, _err, GetWinSocketErrorCodeName(_err), msg, GetWSAErrorMessage(_err));             \
		__debugbreak();                                                  \
	}

#define LENET_MSG_ERROR(msg)                                             \
	{                                                                    \
        ::LimeEngine::Net::NetLogger::NetLogger::LogCore("Error: {}", msg);\
		__debugbreak();                                                  \
	}

#define LENET_LASTERR std::error_code(WSAGetLastError(), std::system_category())

#define LENET_LAST_ERROR() LENET_ERROR(WSAGetLastError())
#define LENET_LAST_ERROR_MSG(msg) LENET_ERROR(WSAGetLastError(), msg)

namespace LimeEngine::Net
{
    using NativeSocket = SOCKET;
    using NativeIOContext = OVERLAPPED;
    using NetBuffer = WSABUF;

    std::string GetWSAErrorMessage(int err);
}
