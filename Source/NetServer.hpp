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

			netEventManager.emplace_back(std::forward<TNetEventHandler>(netEventHandler));
		}
		explicit NetServer(NetSocketIPv4Address address) : serverSocket(NetAddressType::IPv4)
		{
			serverSocket.SetNonblockingMode();
			serverSocket.Bind(address);
			serverSocket.Listen();

			netEventManager.emplace_back();
		}

		template <typename TNetEventHandler = NetEventHandler>
		void AddEventHandler(TNetEventHandler&& netEventHandler)
		{
			netEventManager.emplace_back(std::forward<TNetEventHandler>(netEventHandler));
		}
		void AddEventHandler()
		{
			netEventManager.emplace_back();
		}

		void Update()
		{
			for (auto connectionIter = connections.begin(); connectionIter != connections.end();)
			{
				if (!connectionIter->Update())
					connectionIter = connections.erase(connectionIter);
				else
					++connectionIter;
			}
		}

		void Accept()
		{
			NetSocket clientSocket;
			if (serverSocket.Accept(clientSocket)) { AddConnection(std::move(clientSocket)); }
		}

		void HandleNetEvents()
		{
			for (auto& handler : netEventManager)
			{
				handler.HandleNetEvents();
			}
		}

		void DisconnectAll()
		{
			for (auto& handler : netEventManager)
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
			auto& availableHandler = netEventManager[availableServerIndex];

			size_t newAvailableServerIndex = (availableServerIndex + 1ull) % netEventManager.size();
			if (availableHandler.NumberOfConnections() > netEventManager[newAvailableServerIndex].NumberOfConnections())
			{
				availableServerIndex = newAvailableServerIndex;
			}
			return availableHandler;
		}

	private:
		NetSocket serverSocket;
		std::list<NetConnection> connections;
		std::vector<TNetEventManager> netEventManager;
		size_t availableServerIndex = 0ull;

		std::function<void(NetConnection&)> onConnection;
	};
}
