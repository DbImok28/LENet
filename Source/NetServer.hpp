// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#pragma once
#include "NetEventHandler.hpp"

namespace LimeEngine::Net
{
	template <typename TNetEventManager>
	class NetServer
	{
	public:
		NetServer(const NetServer& other) = delete;
		NetServer operator=(const NetServer& other) = delete;

		template <typename TNetEventHandler = NetEventHandler>
		explicit NetServer(TNetEventHandler&& netEventHandler, NetSocketIPv4Address address) : serverSocket(NetAddressType::IPv4)
		{
			serverSocket.SetNonblockingMode();
			serverSocket.Bind(address);
			serverSocket.Listen();

			netEventManagers.emplace_back(std::forward<TNetEventHandler>(netEventHandler));
		}
		explicit NetServer(NetSocketIPv4Address address) : serverSocket(NetAddressType::IPv4)
		{
			serverSocket.SetNonblockingMode();
			serverSocket.Bind(address);
			serverSocket.Listen();

			netEventManagers.emplace_back();
		}

		template <typename TNetEventHandler = NetEventHandler>
		void AddEventHandler(TNetEventHandler&& netEventHandler)
		{
			netEventManagers.emplace_back(std::forward<TNetEventHandler>(netEventHandler));
		}
		void AddEventHandler()
		{
			netEventManagers.emplace_back();
		}

		void Update()
		{
			for (auto connectionIter = connections.begin(); connectionIter != connections.end();)
			{
				if (!connectionIter->Update())
				{
					connectionIter = connections.erase(connectionIter);
				}
				else
				{
					++connectionIter;
				}
			}
		}

		void Accept()
		{
			NetSocket clientSocket;
			if (serverSocket.Accept(clientSocket)) { AddConnection(std::move(clientSocket)); }
		}

		void HandleNetEvents()
		{
			int index = 0;
			for (auto& handler : netEventManagers)
			{
				NetLogger::LogCore("Handler({})", index++);

				handler.HandleNetEvents();
			}
		}

		void DisconnectAll()
		{
			for (auto& handler : netEventManagers)
			{
				handler.DisconnectAllConnections();
			}
		}

		bool HasConnections() const
		{
			return !connections.empty();
		}
		size_t NumberOfConnections() const
		{
			return connections.size();
		}
		std::list<NetConnection>& GetConnections()
		{
			return connections;
		}

	public:
		void OnConnection(const std::function<void(NetConnection&)>& handler)
		{
			onConnection = handler;
		}

	private:
		void AddConnection(NetSocket&& socket)
		{
			connections.emplace_back();
			auto& connection = connections.back();
			GetAvailableEventHandler().AddConnection(std::move(socket), connection);
			onConnection(connection);
		}
		TNetEventManager& GetAvailableEventHandler()
		{
			auto& availableHandler = netEventManagers[availableServerIndex];

			size_t newAvailableServerIndex = (availableServerIndex + 1ull) % netEventManagers.size();
			if (availableHandler.NumberOfConnections() > netEventManagers[newAvailableServerIndex].NumberOfConnections())
			{
				availableServerIndex = newAvailableServerIndex;
			}
			return availableHandler;
		}

	private:
		NetSocket serverSocket;
		std::list<NetConnection> connections;
		std::vector<TNetEventManager> netEventManagers;
		size_t availableServerIndex = 0ull;

		std::function<void(NetConnection&)> onConnection;
	};
}
