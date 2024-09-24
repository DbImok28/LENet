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


void ShowWSAErrorMessage(int err)
{
    char* s = nullptr;
    FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&s, 0, nullptr);
    std::cout << "Desc: " << s << std::endl;
    LocalFree(s);
}

#define LENET_ERROR(err, msg)                                            \
	{                                                                    \
		int _err = err;                                                  \
		std::cout << "failed: code " << _err << " " << msg << std::endl; \
		ShowWSAErrorMessage(_err);                                       \
		__debugbreak();                                                  \
	}

#define LENET_MSG_ERROR(msg)                                             \
	{                                                                    \
		std::cout << "failed: " << msg << std::endl;                    \
		__debugbreak();                                                  \
	}

#define LENET_LASTERR std::error_code(WSAGetLastError(), std::system_category())
