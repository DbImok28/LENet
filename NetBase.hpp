#pragma once
#define NOMINMAX

#define LENET_WIN32

#include <iostream>
#include <array>
#include <vector>
#include <functional>
#include <format>
#include <chrono>

#ifdef LENET_WIN32
#define FD_SETSIZE 1024
#endif
#include <winsock2.h>
#include <WS2tcpip.h>
#include "WinSocketError.hpp"
#include "NetLogger.hpp"

namespace LimeEngine::Net
{
    std::string GetWSAErrorMessage(int err)
    {
        char *s = nullptr;
        FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&s), 0, nullptr);
        std::string msg = s;
        LocalFree(s);
        return msg;
    }
}

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
