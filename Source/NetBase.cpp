// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#include "NetBase.hpp"

namespace LimeEngine::Net
{
	std::string GetWSAErrorMessage(int err)
	{
		char* s = nullptr;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&s),
			0,
			nullptr);
		std::string msg = s;
		LocalFree(s);
		return msg;
	}
}
