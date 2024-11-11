// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#include "WinSocketError.hpp"
#include <WinSock2.h>

#define MACRO_TO_STR_CASE(macro) \
	case macro: return #macro;

std::string GetWinSocketErrorCodeName(int err)
{
	switch (err)
	{
		MACRO_TO_STR_CASE(WSA_INVALID_HANDLE)
		MACRO_TO_STR_CASE(WSA_NOT_ENOUGH_MEMORY)
		MACRO_TO_STR_CASE(WSA_INVALID_PARAMETER)
		MACRO_TO_STR_CASE(WSA_OPERATION_ABORTED)
		MACRO_TO_STR_CASE(WSA_IO_INCOMPLETE)
		MACRO_TO_STR_CASE(WSA_IO_PENDING)
		MACRO_TO_STR_CASE(WSAEINTR)
		MACRO_TO_STR_CASE(WSAEBADF)
		MACRO_TO_STR_CASE(WSAEACCES)
		MACRO_TO_STR_CASE(WSAEFAULT)
		MACRO_TO_STR_CASE(WSAEINVAL)
		MACRO_TO_STR_CASE(WSAEMFILE)
		MACRO_TO_STR_CASE(WSAEWOULDBLOCK)
		MACRO_TO_STR_CASE(WSAEINPROGRESS)
		MACRO_TO_STR_CASE(WSAEALREADY)
		MACRO_TO_STR_CASE(WSAENOTSOCK)
		MACRO_TO_STR_CASE(WSAEDESTADDRREQ)
		MACRO_TO_STR_CASE(WSAEMSGSIZE)
		MACRO_TO_STR_CASE(WSAEPROTOTYPE)
		MACRO_TO_STR_CASE(WSAENOPROTOOPT)
		MACRO_TO_STR_CASE(WSAEPROTONOSUPPORT)
		MACRO_TO_STR_CASE(WSAESOCKTNOSUPPORT)
		MACRO_TO_STR_CASE(WSAEOPNOTSUPP)
		MACRO_TO_STR_CASE(WSAEPFNOSUPPORT)
		MACRO_TO_STR_CASE(WSAEAFNOSUPPORT)
		MACRO_TO_STR_CASE(WSAEADDRINUSE)
		MACRO_TO_STR_CASE(WSAEADDRNOTAVAIL)
		MACRO_TO_STR_CASE(WSAENETDOWN)
		MACRO_TO_STR_CASE(WSAENETUNREACH)
		MACRO_TO_STR_CASE(WSAENETRESET)
		MACRO_TO_STR_CASE(WSAECONNABORTED)
		MACRO_TO_STR_CASE(WSAECONNRESET)
		MACRO_TO_STR_CASE(WSAENOBUFS)
		MACRO_TO_STR_CASE(WSAEISCONN)
		MACRO_TO_STR_CASE(WSAENOTCONN)
		MACRO_TO_STR_CASE(WSAESHUTDOWN)
		MACRO_TO_STR_CASE(WSAETOOMANYREFS)
		MACRO_TO_STR_CASE(WSAETIMEDOUT)
		MACRO_TO_STR_CASE(WSAECONNREFUSED)
		MACRO_TO_STR_CASE(WSAELOOP)
		MACRO_TO_STR_CASE(WSAENAMETOOLONG)
		MACRO_TO_STR_CASE(WSAEHOSTDOWN)
		MACRO_TO_STR_CASE(WSAEHOSTUNREACH)
		MACRO_TO_STR_CASE(WSAENOTEMPTY)
		MACRO_TO_STR_CASE(WSAEPROCLIM)
		MACRO_TO_STR_CASE(WSAEUSERS)
		MACRO_TO_STR_CASE(WSAEDQUOT)
		MACRO_TO_STR_CASE(WSAESTALE)
		MACRO_TO_STR_CASE(WSAEREMOTE)
		MACRO_TO_STR_CASE(WSASYSNOTREADY)
		MACRO_TO_STR_CASE(WSAVERNOTSUPPORTED)
		MACRO_TO_STR_CASE(WSANOTINITIALISED)
		MACRO_TO_STR_CASE(WSAEDISCON)
		MACRO_TO_STR_CASE(WSAENOMORE)
		MACRO_TO_STR_CASE(WSAECANCELLED)
		MACRO_TO_STR_CASE(WSAEINVALIDPROCTABLE)
		MACRO_TO_STR_CASE(WSAEINVALIDPROVIDER)
		MACRO_TO_STR_CASE(WSAEPROVIDERFAILEDINIT)
		MACRO_TO_STR_CASE(WSASYSCALLFAILURE)
		MACRO_TO_STR_CASE(WSASERVICE_NOT_FOUND)
		MACRO_TO_STR_CASE(WSATYPE_NOT_FOUND)
		MACRO_TO_STR_CASE(WSA_E_NO_MORE)
		MACRO_TO_STR_CASE(WSA_E_CANCELLED)
		MACRO_TO_STR_CASE(WSAEREFUSED)
		MACRO_TO_STR_CASE(WSAHOST_NOT_FOUND)
		MACRO_TO_STR_CASE(WSATRY_AGAIN)
		MACRO_TO_STR_CASE(WSANO_RECOVERY)
		MACRO_TO_STR_CASE(WSANO_DATA)
		MACRO_TO_STR_CASE(WSA_QOS_RECEIVERS)
		MACRO_TO_STR_CASE(WSA_QOS_SENDERS)
		MACRO_TO_STR_CASE(WSA_QOS_NO_SENDERS)
		MACRO_TO_STR_CASE(WSA_QOS_NO_RECEIVERS)
		MACRO_TO_STR_CASE(WSA_QOS_REQUEST_CONFIRMED)
		MACRO_TO_STR_CASE(WSA_QOS_ADMISSION_FAILURE)
		MACRO_TO_STR_CASE(WSA_QOS_POLICY_FAILURE)
		MACRO_TO_STR_CASE(WSA_QOS_BAD_STYLE)
		MACRO_TO_STR_CASE(WSA_QOS_BAD_OBJECT)
		MACRO_TO_STR_CASE(WSA_QOS_TRAFFIC_CTRL_ERROR)
		MACRO_TO_STR_CASE(WSA_QOS_GENERIC_ERROR)
		MACRO_TO_STR_CASE(WSA_QOS_ESERVICETYPE)
		MACRO_TO_STR_CASE(WSA_QOS_EFLOWSPEC)
		MACRO_TO_STR_CASE(WSA_QOS_EPROVSPECBUF)
		MACRO_TO_STR_CASE(WSA_QOS_EFILTERSTYLE)
		MACRO_TO_STR_CASE(WSA_QOS_EFILTERTYPE)
		MACRO_TO_STR_CASE(WSA_QOS_EFILTERCOUNT)
		MACRO_TO_STR_CASE(WSA_QOS_EOBJLENGTH)
		MACRO_TO_STR_CASE(WSA_QOS_EFLOWCOUNT)
		MACRO_TO_STR_CASE(WSA_QOS_EUNKOWNPSOBJ)
		MACRO_TO_STR_CASE(WSA_QOS_EPOLICYOBJ)
		MACRO_TO_STR_CASE(WSA_QOS_EFLOWDESC)
		MACRO_TO_STR_CASE(WSA_QOS_EPSFLOWSPEC)
		MACRO_TO_STR_CASE(WSA_QOS_EPSFILTERSPEC)
		MACRO_TO_STR_CASE(WSA_QOS_ESDMODEOBJ)
		MACRO_TO_STR_CASE(WSA_QOS_ESHAPERATEOBJ)
		MACRO_TO_STR_CASE(WSA_QOS_RESERVED_PETYPE)
		default: return "Unknown error code";
	}
}
