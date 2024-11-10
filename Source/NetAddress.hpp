#pragma once
#include "NetBase.hpp"

namespace LimeEngine::Net
{
	enum class NetAddressType
	{
		IPv4 = AF_INET,
		IPv6 = AF_INET6
	};

	class NetIPv4Address
	{
	public:
		NetIPv4Address(const std::string& addr)
		{
			Set(addr);
		}

		void Set(const std::string& addr)
		{
			int err = inet_pton(AF_INET, addr.c_str(), &inAddr);
			if (err <= 0) { std::cout << "Error in IPv4 translation to special numeric format" << std::endl; }
		}
		std::string ToString()
		{
			char buf[INET_ADDRSTRLEN];
			if (inet_ntop(AF_INET, &inAddr, buf, sizeof(buf)) == nullptr)
			{
				std::cout << "Error in IPv4 translation to string format " << WSAGetLastError() << std::endl;
				return "invalid";
			}
			return buf;
		}

	private:
		in_addr inAddr;
	};

	class NetIPv6Address
	{
	public:
		NetIPv6Address(const std::string& addr)
		{
			Set(addr);
		}

		void Set(const std::string& addr)
		{
			int err = inet_pton(AF_INET6, addr.c_str(), &inAddr);
			if (err <= 0) { LENET_ERROR(WSAGetLastError(), "Can't translate IPv6 to special numeric format"); }
		}
		std::string ToString()
		{
			char buf[INET6_ADDRSTRLEN];
			if (inet_ntop(AF_INET6, &inAddr, buf, sizeof(buf)) == nullptr)
			{
				LENET_ERROR(WSAGetLastError(), "Can't translate IPv6 to string format");
				return "invalid";
			}
			return buf;
		}

	private:
		in6_addr inAddr;
	};

	class NetSocketIPv4Address
	{
	public:
		NetSocketIPv4Address(NetIPv4Address ip, uint16_t port)
		{
			addr.sin_family = AF_INET;
			addr.sin_addr = *reinterpret_cast<in_addr*>(&ip);
			addr.sin_port = htons(port);
		}
		NetSocketIPv4Address(uint16_t port)
		{
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = htons(port);
		}
		// std::string GetHostName()
		//{
		//     return gethostbyname(addr);
		// }
		std::string ToString()
		{
			return std::format("{}:{}", reinterpret_cast<NetIPv4Address*>(&addr.sin_addr)->ToString(), htons(addr.sin_port));
		}

	private:
		sockaddr_in addr;
	};

	class NetSocketIPv6Address
	{
	public:
		NetSocketIPv6Address(NetIPv6Address ip, uint16_t port)
		{
			addr.sin6_family = AF_INET6;
			addr.sin6_addr = *reinterpret_cast<in6_addr*>(&ip);
			addr.sin6_port = htons(port);
		}
		// std::string GetHostName()
		//{
		//     return gethostbyname(addr);
		// }

	private:
		sockaddr_in6 addr;
	};
}
