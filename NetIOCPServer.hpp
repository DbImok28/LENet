#pragma once
#include <functional>
#include "NetPollServer.hpp"
#include <cstddef>
#include "BufferPool.hpp"

namespace LimeEngine::Net
{
	enum class IOOperationType
	{
		Send,
		Receive
	};

	struct IOContext
	{
		explicit IOContext(IOOperationType operationType) noexcept : operationType(operationType)
		{
			wsaBuf.len = 0;
			wsaBuf.buf = nullptr;

			overlapped.Internal = 0;
			overlapped.InternalHigh = 0;
			overlapped.Offset = 0;
			overlapped.OffsetHigh = 0;
			overlapped.hEvent = nullptr;
		}
		~IOContext() noexcept
		{
			std::cout << "~IOContext()" << std::endl;
		}

		static IOContext* FromOverlapped(LPOVERLAPPED overlapped) noexcept
		{
			return CONTAINING_RECORD(overlapped, IOContext, overlapped);
		}

//		void SetBuffer(const char* buffer)
//		{
//			wsaBuf.buf = const_cast<char*>(buffer);
//		}
//		void SetBuffer(char* buffer)
//		{
//			wsaBuf.buf = buffer;
//		}
        void SetNextBuffer(const char* buffer)
        {
            SetNextBuffer(const_cast<char*>(buffer));
        }
        void SetNextBuffer(char* buffer)
        {
            wsaBuf.buf = buffer;
            buffers.emplace_back(buffer);
        }

        std::pair<char*, uint32_t> GetBuffer()
        {
            return std::make_pair(wsaBuf.buf, wsaBuf.len);
        }
        const std::list<char*>& GetBuffers()
        {
            return buffers;
        }
        void SetMessageLength(uint32_t messageLen)
        {
            wsaBuf.len = messageLen;
        }
        void Reset()
        {
            buffers.clear();
        }

		OVERLAPPED overlapped;
		WSABUF wsaBuf;
        std::list<char*> buffers;

		//static constexpr size_t len = 256;
		IOOperationType operationType = IOOperationType::Receive;
	};

	struct SocketContext
	{
		SocketContext(NetSocket&& socket, NetConnection& connection) : socket(std::move(socket)), connection(connection) {}
		~SocketContext() noexcept
		{
			std::cout << "~SocketContext()" << std::endl;
		}

		NetSocket socket;
		NetConnection& connection;
		IOContext receiveContext{ IOOperationType::Receive };
		IOContext sendContext{ IOOperationType::Send };
	};

	template <typename TKey>
	class IOCompletionPort
	{
	public:
		IOCompletionPort()
		{
			completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
			if (completionPort == NULL) { LENET_ERROR(WSAGetLastError(), "Can't to create IOCompletionPort"); }
			//std::cout << "Create IOCP " << completionPort << std::endl;
		}
		~IOCompletionPort()
		{
			std::cout << "~IOCompletionPort. Close completionPort" << std::endl;
			CloseHandle(completionPort);
		}

		void PostCloseStatus()
		{
			PostQueuedCompletionStatus(completionPort, 0, 0, nullptr);
		}

		//        void StartThreads()
		//        {
		//            SYSTEM_INFO SystemInfo;
		//            GetSystemInfo(&SystemInfo);
		//
		//            for (int i = 0; i < SystemInfo.dwNumberOfProcessors; i++)
		//            {
		//
		//                HANDLE ThreadHandle;
		//
		//                // Create a server worker thread, and pass the
		//                // completion port to the thread. NOTE: the
		//                // ServerWorkerThread procedure is not defined in this listing.
		//
		//                ThreadHandle = CreateThread(NULL, 0, ServerWorkerThread, CompletionPort, 0, NULL;
		//
		//                // Close the thread handle
		//
		//                CloseHandle(ThreadHandle);
		//
		//            }
		//        }

		void Add(HANDLE handle, TKey* key)
		{
			if (CreateIoCompletionPort(handle, completionPort, reinterpret_cast<ULONG_PTR>(key), 0) == NULL)
			{
				LENET_ERROR(WSAGetLastError(), "Can't to add socket to IoCompletionPort");
			}
			//std::cout << "Add handle to IOCP " << handle << std::endl;
		}
		void Add(SOCKET socket, TKey* key)
		{
			Add(reinterpret_cast<HANDLE>(socket), key);
		}

		bool Wait(uint32_t timeout, uint32_t& outBytesTransferred, TKey*& outKey, IOContext*& outContext)
		{ // INFINITE
			static_assert(offsetof(IOContext, overlapped) == 0, "The overlapped field must be first");

			std::cout << "Wait " << timeout << "ms" << std::endl;
			//uint32_t bytesTransferred = 0;
			if (!GetQueuedCompletionStatus(
					completionPort, reinterpret_cast<LPDWORD>(&outBytesTransferred), reinterpret_cast<PULONG_PTR>(&outKey), reinterpret_cast<LPOVERLAPPED*>(&outContext), timeout))
			{
				auto err = GetLastError();
				if (err == WAIT_TIMEOUT) return false;

				if (err == ERROR_NETNAME_DELETED || err == ERROR_CONNECTION_ABORTED)
				{
					outBytesTransferred = 0;
					return true;
				}

				std::cout << "GetQueuedCompletionStatus failed: " << err << std::endl;
				LENET_ERROR(err, "Can't to get CompletionStatus");
				return false;
			}
			//std::cout << "MSG Status b" << outBytesTransferred << std::endl;
			//outContext->bytesTransferred = bytesTransferred;
			return true;
		}
		// void ReadCompleted(NetSocket& socket, IOBuffer buffer)
		// {
		//
		// }

	private:
		HANDLE completionPort = INVALID_HANDLE_VALUE;
	};

	struct NetTCPAsyncDataHandler
	{
		static void ReceiveAsync(NetSocket& socket, IOContext& context)
		{
			DWORD flags = 0;
			//if (WSARecv(socket.GetSocket(), &context.wsaBuf, 1, &context.bytesTransferred, &flags, &context.overlapped, nullptr) == SOCKET_ERROR)
			if (WSARecv(socket.GetSocket(), &context.wsaBuf, 1, nullptr, &flags, &context.overlapped, nullptr) == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err == WSA_IO_PENDING)
					std::cout << "[Async Receive started]" << std::endl;
				else
					LENET_ERROR(err, "Can't to receive async message from Client");
			}
		}
		static void SendAsync(NetSocket& socket, IOContext& context)
		{
			DWORD flags = 0;
			//if (WSASend(socket.GetSocket(), &context.wsaBuf, 1, &context.bytesTransferred, flags, &context.overlapped, nullptr) == SOCKET_ERROR)
			if (WSASend(socket.GetSocket(), &context.wsaBuf, 1, nullptr, flags, &context.overlapped, nullptr) == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err == WSA_IO_PENDING)
					std::cout << "Async Send started" << std::endl;
				else
					LENET_ERROR(err, "Can't to send async message to Client");
			}
		}
	};

	template <typename TNetDataHandler>
	class NetIOCPManager
	{
	public:
		NetIOCPManager() = default;
		~NetIOCPManager() noexcept
		{
			std::cout << "~NetIOCPManager()" << std::endl;
		}

		void AddConnection(NetSocket&& socket, NetConnection& connection)
		{
			auto& socketContext = socketContexts.emplace_back(std::make_unique<SocketContext>(std::move(socket), connection));
			completionPort.Add(socketContext->socket.GetSocket(), socketContext.get());

            socketContext->receiveContext.SetMessageLength(bufferPool.size);
            socketContext->receiveContext.SetNextBuffer(bufferPool.TakeBuffer());

			TNetDataHandler::ReceiveAsync(socketContext->socket, socketContext->receiveContext);
		}
		void RemoveConnection(SocketContext* socketContext)
		{
			auto socketContextIter = std::find_if(
				std::begin(socketContexts), std::end(socketContexts), [socketContext](const std::unique_ptr<SocketContext>& item) { return item.get() == socketContext; });
			if (socketContextIter != std::end(socketContexts))
			{
				socketContext->connection.ChangeStateToClose();
				socketContexts.erase(socketContextIter);
			}
		}

		void ProcessSend()
		{
			for (auto& socketContext : socketContexts)
			{
				if (!socketContext->connection.messagesToSend.empty())
				{
					auto& sendMsg = socketContext->connection.messagesToSend.back();
					if (sendMsg.sended) continue;

                    socketContext->sendContext.SetMessageLength(sendMsg.msg.length() + 1);
					socketContext->sendContext.SetNextBuffer(sendMsg.msg.c_str());
					sendMsg.sended = true;
					TNetDataHandler::SendAsync(socketContext->socket, socketContext->sendContext);
				}
			}
		}

		void HandleNetEvents()
		{
			ProcessSend();

			uint32_t bytesTransferred;
			SocketContext* socketContext = nullptr;
			IOContext* ioContext = nullptr;
			if (!completionPort.Wait(100, bytesTransferred, socketContext, ioContext)) return;

			std::cout << "[Async action complete]" << std::endl;

			//auto& buf = socketContext->receiveContext.buf;
			if (bytesTransferred == 0)
			{
				std::cout << "[Net] Client disconnected: " << socketContext->socket.GetId() << std::endl;
				RemoveConnection(socketContext);
			}
			else if (ioContext->operationType == IOOperationType::Receive) // Read
			{
                auto [msg, size] = ioContext->GetBuffer();
				std::cout << "[con " << socketContext->connection.GetId() << "][soc " << socketContext->socket.GetId() << "][recv " << bytesTransferred << "b]"
                        << std::endl;

                if (msg[bytesTransferred - 1] == '\0')
                {
                    std::ostringstream oss;
                    for (auto& buffer : ioContext->GetBuffers())
                    {
                        oss << buffer;
                    }
                    socketContext->connection.receivedMessages.emplace(oss.str());
                    std::cout << "[msg end]" << std::endl;
                    for (auto& buffer : ioContext->GetBuffers())
                    {
                        bufferPool.ReturnBuffer(buffer);
                    }
                    ioContext->Reset();
                }

                ioContext->SetNextBuffer(bufferPool.TakeBuffer());
				TNetDataHandler::ReceiveAsync(socketContext->socket, socketContext->receiveContext);
			}
			else if (ioContext->operationType == IOOperationType::Send) // Write
			{
				std::cout << "[con " << socketContext->connection.GetId() << "][soc " << socketContext->socket.GetId() << "][send " << bytesTransferred << "b]"
						  << std::endl;

				socketContext->connection.messagesToSend.pop();
                ioContext->Reset();
				if (!socketContext->connection.messagesToSend.empty())
				{
					auto& sendMsg = socketContext->connection.messagesToSend.back();
                    ioContext->SetMessageLength(sendMsg.msg.length() + 1);
					ioContext->SetNextBuffer(sendMsg.msg.c_str());
					sendMsg.sended = true;
					TNetDataHandler::SendAsync(socketContext->socket, socketContext->sendContext);
				}
			}
		}

	private:
		IOCompletionPort<SocketContext> completionPort;
		std::vector<std::unique_ptr<SocketContext>> socketContexts;
        BufferPool<1024> bufferPool;
	};

	template <typename TNetEventHandler>
	class NetTCPIOCPServer
	{
	public:
		explicit NetTCPIOCPServer(NetSocketIPv4Address address) : serverSocket(NetAddressType::IPv4, true)
		{
			serverSocket.SetNonblockingMode();
			serverSocket.Bind(address);
			serverSocket.Listen();
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
		bool Accept()
		{
			NetSocket clientSocket;
			if (serverSocket.Accept(clientSocket))
			{
				AddConnection(std::move(clientSocket));
				return true;
			}
			return false;
		}

		/*
         g_hAcceptEvent = WSACreateEvent();

             if (WSA_INVALID_EVENT == g_hAcceptEvent)
             {
                  printf("\nError occurred while WSACreateEvent().");
                  goto error;
             }

             if (SOCKET_ERROR == WSAEventSelect(ListenSocket,
                                g_hAcceptEvent, FD_ACCEPT))
             {
                  printf("\nError occurred while WSAEventSelect().");
                  WSACloseEvent(g_hAcceptEvent);
                  goto error;
             }

        //This thread will look for accept event
        DWORD WINAPI AcceptThread(LPVOID lParam)
        {
             SOCKET ListenSocket = (SOCKET)lParam;

             WSANETWORKEVENTS WSAEvents;

             //Accept thread will be around to look for
             //accept event, until a Shutdown event is not Signaled.
             while(WAIT_OBJECT_0 !=
                   WaitForSingleObject(g_hShutdownEvent, 0))
             {
                  if (WSA_WAIT_TIMEOUT != WSAWaitForMultipleEvents(1,
                      &g_hAcceptEvent, FALSE, WAIT_TIMEOUT_INTERVAL, FALSE))
                  {
                       WSAEnumNetworkEvents(ListenSocket,
                             g_hAcceptEvent, &WSAEvents);
                       if ((WSAEvents.lNetworkEvents & FD_ACCEPT) &&
                           (0 == WSAEvents.iErrorCode[FD_ACCEPT_BIT]))
                       {
                            //Process it.
                            AcceptConnection(ListenSocket);
                       }
                  }
             }

             return 0;
        }
         * */

		void HandleNetEvents()
		{
			server.HandleNetEvents();
		}
		void OnConnection(const std::function<void(NetConnection&)>& handler)
		{
			onConnection = handler;
		}

	private:
		void AddConnection(NetSocket&& socket)
		{
			connections.emplace_back();
			auto& connection = connections.back();
			server.AddConnection(std::move(socket), connection);
			onConnection(connection);
		}

	public:
		//private:
		NetSocket serverSocket;
		std::list<NetConnection> connections;
		NetIOCPManager<NetTCPAsyncDataHandler> server;

		std::function<void(NetConnection&)> onConnection;
	};
}
