// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#pragma once
#include <functional>
#include <cstddef>
#include "BufferPool.hpp"
#include "NetEventHandler.hpp"

namespace LimeEngine::Net
{
	template <typename TKey, typename TContext>
	class IOCompletionPort
	{
	public:
		IOCompletionPort()
		{
			completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
			if (completionPort == nullptr) { LENET_LAST_ERROR_MSG("Can't to create IOCompletionPort"); }
		}
		~IOCompletionPort()
		{
			CloseHandle(completionPort);
		}

		void PostCloseStatus()
		{
			PostQueuedCompletionStatus(completionPort, 0, 0, nullptr);
		}

		void Add(HANDLE handle, TKey* key)
		{
			if (CreateIoCompletionPort(handle, completionPort, reinterpret_cast<ULONG_PTR>(key), 0) == NULL) { LENET_LAST_ERROR_MSG("Can't to add socket to IoCompletionPort"); }
		}
		void Add(NativeSocket socket, TKey* key)
		{
			Add(reinterpret_cast<HANDLE>(socket), key);
		}

		bool Wait(uint32_t timeout, uint32_t& outBytesTransferred, TKey*& outKey, TContext*& outContext)
		{ // INFINITE
			static_assert(offsetof(TContext, nativeIoContext) == 0, "NativeIoContext field must be first");

			NetLogger::LogCore("Wait {}ms", timeout);

			if (!GetQueuedCompletionStatus(
					completionPort,
					reinterpret_cast<LPDWORD>(&outBytesTransferred),
					reinterpret_cast<PULONG_PTR>(&outKey),
					reinterpret_cast<NativeIOContext**>(&outContext),
					timeout))
			{
				auto err = GetLastError();

				if (err == WAIT_TIMEOUT) return false;
				if (err == ERROR_NETNAME_DELETED || err == ERROR_CONNECTION_ABORTED)
				{
					outBytesTransferred = 0;
					return true;
				}

				NetLogger::LogCore("GetQueuedCompletionStatus failed: {}", err);
				LENET_ERROR(err, "Can't to get CompletionStatus");
				return false;
			}
			if (outContext == nullptr) return false;
			return true;
		}

	private:
		HANDLE completionPort = INVALID_HANDLE_VALUE;
	};

	template <typename TNetProtocol, typename TNetEventHandler = NetEventHandler>
	class NetIOCPEventManager
	{
	public:
		NetIOCPEventManager(const NetIOCPEventManager& other) = delete;
		NetIOCPEventManager operator=(const NetIOCPEventManager& other) = delete;


		NetIOCPEventManager(NetIOCPEventManager&& other) noexcept :
			netEventHandler(std::move(other.netEventHandler)), completionPort(std::move(other.completionPort)), socketContexts(std::move(other.socketContexts))
		{}
		NetIOCPEventManager& operator=(NetIOCPEventManager&& other) noexcept
		{
			if (this != &other)
			{
				netEventHandler= std::move(other.netEventHandler);
				completionPort = std::move(other.completionPort);
				socketContexts = std::move(other.socketContexts);
			}
			return *this;
		}

		NetIOCPEventManager() = default;
		explicit NetIOCPEventManager(TNetEventHandler&& netEventHandler) : netEventHandler(std::move(netEventHandler)) {};

	public:
		void AddConnection(NetSocket&& socket, NetConnection& connection)
		{
			auto& socketContext = socketContexts.emplace_back(std::make_unique<SocketContext>(std::move(socket), &connection));
			completionPort.Add(socketContext->socket.GetNativeSocket(), socketContext.get());
			netEventHandler.StartRead(*socketContext);

			TNetProtocol::ReceiveAsync(socketContext->socket, &socketContext->receiveContext.netBuffer, &socketContext->receiveContext.nativeIoContext);
		}

		void DisconnectAllConnections()
		{
			completionPort.PostCloseStatus();
			HandleNetEvents();

			for (auto& socketContext : socketContexts)
			{
				netEventHandler.Disconnect(*socketContext);
			}
			socketContexts.clear();
		}

	private:
		void RemoveConnection(SocketContext* socketContext)
		{
			auto socketContextIter = std::find_if(
				std::begin(socketContexts), std::end(socketContexts), [socketContext](const std::unique_ptr<SocketContext>& item) { return item.get() == socketContext; });
			if (socketContextIter != std::end(socketContexts))
			{
				if (netEventHandler.Disconnect(*socketContext)) { socketContexts.erase(socketContextIter); }
			}
		}

		void ProcessSend()
		{
			for (auto& socketContext : socketContexts)
			{
				if (netEventHandler.StartWrite(*socketContext))
				{
					TNetProtocol::SendAsync(socketContext->socket, &socketContext->sendContext.netBuffer, &socketContext->sendContext.nativeIoContext);
				}
			}
		}

	public:
		void HandleNetEvents()
		{
			ProcessSend();

			uint32_t bytesTransferred;
			SocketContext* socketContext = nullptr;
			IOContext* ioContext = nullptr;

			if (!completionPort.Wait(100, bytesTransferred, socketContext, ioContext)) return;

			if (bytesTransferred == 0) { RemoveConnection(socketContext); }
			// Read
			else if (ioContext->operationType == IOOperationType::Receive)
			{
				netEventHandler.Read(*socketContext, bytesTransferred);
				TNetProtocol::ReceiveAsync(socketContext->socket, &socketContext->receiveContext.netBuffer, &socketContext->receiveContext.nativeIoContext);
			}
			// Write
			else if (ioContext->operationType == IOOperationType::Send)
			{
				if (netEventHandler.Write(*socketContext, bytesTransferred))
				{
					TNetProtocol::SendAsync(socketContext->socket, &socketContext->sendContext.netBuffer, &socketContext->sendContext.nativeIoContext);
				}
			}
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
		IOCompletionPort<SocketContext, IOContext> completionPort;
		std::vector<std::unique_ptr<SocketContext>> socketContexts;
	};
}
