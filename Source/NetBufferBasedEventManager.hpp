// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#pragma once
#include "NetEventHandler.hpp"

namespace LimeEngine::Net
{
	template <typename TNetEventBuffer, typename TNetProtocol, typename TNetEventHandler = NetEventHandler>
	class NetBufferBasedEventManager
	{
	public:
		NetBufferBasedEventManager(const NetBufferBasedEventManager& other) = delete;
		NetBufferBasedEventManager operator=(const NetBufferBasedEventManager& other) = delete;

		NetBufferBasedEventManager(NetBufferBasedEventManager&& other) noexcept :
			netEventHandler(std::move(other.netEventHandler)), netEventBuffer(std::move(other.netEventBuffer)), socketContexts(std::move(other.socketContexts))
		{}
		NetBufferBasedEventManager& operator=(NetBufferBasedEventManager&& other) noexcept
		{
			if (this != &other)
			{
				netEventHandler = std::move(other.netEventHandler);
				netEventBuffer = std::move(other.netEventBuffer);
				socketContexts = std::move(other.socketContexts);
			}
			return *this;
		}
		NetBufferBasedEventManager() noexcept = default;
		explicit NetBufferBasedEventManager(TNetEventHandler&& netEventHandler) : netEventHandler(std::move(netEventHandler)) {};

	public:
		void AddConnection(NetSocket&& socket, NetConnection& connection)
		{
			auto& socketContext = socketContexts.emplace_back(std::make_unique<SocketContext>(std::move(socket), &connection));
			netEventBuffer.Add(socketContext->socket.GetNativeSocket());
			netEventHandler.StartRead(*socketContext);
		}

		void DisconnectAllConnections()
		{
			for (auto& socketContext : socketContexts)
			{
				netEventHandler.Disconnect(*socketContext);
			}
			socketContexts.clear();
		}

	private:
		void RemoveConnection(size_t index)
		{
			if (netEventHandler.Disconnect(*socketContexts[index]))
			{
				netEventBuffer.Remove(index);
				socketContexts.erase(std::begin(socketContexts) + index);
			}
		}
		void RemoveConnection(SocketContext* socketContext)
		{
			auto socketContextIter = std::find_if(
				std::begin(socketContexts), std::end(socketContexts), [socketContext](const std::unique_ptr<SocketContext>& item) { return item.get() == socketContext; });
			if (socketContextIter != std::end(socketContexts))
			{
				if (netEventHandler.Disconnect(*socketContext))
				{
					netEventBuffer.Remove(std::distance(std::begin(socketContexts), socketContextIter));
					socketContexts.erase(socketContextIter);
				}
			}
		}

		void ProcessSend()
		{
			for (size_t i = 0; i < netEventBuffer.Count(); ++i)
			{
				if (netEventHandler.StartWrite(*socketContexts[i])) { netEventBuffer.SetWriteFlag(i); }
			}
		}

	public:
		bool HandleNetEvents(uint32_t timeout = 1u)
		{
			if (netEventBuffer.Empty()) return true;

			ProcessSend();

			int pollResult = netEventBuffer.WaitForEvents(timeout);
			if (pollResult == 0) return true;

			netEventBuffer.Log();

			for (int i = 0; i < netEventBuffer.Count(); ++i)
			{
				if (pollResult == 0) break;

				SocketContext& socketContext = *socketContexts[i];
				auto&& netEvent = netEventBuffer.At(i);

				if (netEvent.IsChanged()) --pollResult;

				//  Read
				if (netEvent.CheckRead())
				{
					int bytesTransferred;
					NetBuffer& netBuffer = socketContext.receiveContext.netBuffer;
					if (TNetProtocol::Receive(socketContext.socket, netBuffer.buf, netBuffer.len, bytesTransferred)) { netEventHandler.Read(socketContext, bytesTransferred); }
					else
					{
						NetLogger::LogCore("Receive=0, Client {} disconnected", socketContext.socket.GetId());
						RemoveConnection(i);
						continue;
					}
				}

				// Write
				if (netEvent.CheckWrite() && netEventHandler.ReadyToWrite(socketContext))
				{
					int bytesTransferred;
					NetBuffer& netBuffer = socketContext.sendContext.netBuffer;
					if (TNetProtocol::Send(socketContext.socket, netBuffer.buf, netBuffer.len, bytesTransferred))
					{
						if (!netEventHandler.Write(socketContext, bytesTransferred)) { netEventBuffer.ResetWriteFlag(i); }
					}
					else
					{
						NetLogger::LogCore("Send=0, Client {} disconnected", socketContext.socket.GetId());
						RemoveConnection(i);
						continue;
					}
				}

				// Disconnections
				if (netEvent.CheckDisconnect())
				{
					NetLogger::LogCore("Client {} disconnected", socketContext.socket.GetId());
					RemoveConnection(i);
					continue;
				}

				// Exceptions
				if (netEvent.CheckExcept())
				{
					LENET_LAST_ERROR_MSG("Unable to read from client");
					RemoveConnection(i);
					continue;
				}
			}
			return true;
		}

		bool HasConnections() const
		{
			return !socketContexts.empty();
		}
		size_t NumberOfConnections() const
		{
			return socketContexts.size();
		}

	private:
		TNetEventHandler netEventHandler;
		TNetEventBuffer netEventBuffer;
		std::vector<std::unique_ptr<SocketContext>> socketContexts;
	};
}
