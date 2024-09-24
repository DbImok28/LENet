#pragma once

class NetUDPClientBase : public NetBase
{
public:
    NetUDPClientBase(NetAddressType addressType)
        : NetBase(::WSASocket(static_cast<int>(addressType), SOCK_DGRAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED))
    {
        if (_socket == INVALID_SOCKET)
        {
            std::cout << "Error initialization socket # " << WSAGetLastError() << std::endl;
            status = NetStatus::Error;
        }
    }
};