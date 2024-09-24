#pragma once

#include "NetSockets.hpp"
#include "NetEventManager.hpp"
#include "NetIOCPServer.hpp"

namespace LimeEngine::Net::EchoServer
{
	template <size_t time>
	void TimedTask(std::function<void()> task)
	{
		static auto start = std::chrono::system_clock::now();
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		if (elapsed_seconds.count() > time)
		{
			task();
			start = std::chrono::system_clock::now();
		}
	}

	bool LogReceive(NetSocket& socket)
	{
		std::array<char, 256> buf;
		int bytesReceived;
		if (socket.Receive(buf.data(), buf.size(), bytesReceived))
		{
			std::cout << "[soc " << socket.GetId() << "][recv " << bytesReceived << "b] " << buf.data() << std::endl;
			return true;
		}
		//std::cout << "[receive " << bytesReceived << "b fail]: " << std::endl;

//        BufferPool<256> bufferPool;
//        std::string msg;
//        if (socket.Receive(bufferPool, msg))
//        {
//            std::cout << "[soc " << socket.GetId() << "][recv " << msg.size() << "b] " << msg << std::endl;
//            return true;
//        }

        return false;
	}

	bool LogSend(NetSocket& socket, const std::string& msg = "Hello")
	{
		int bytesSent;
		if (socket.Send(msg.c_str(), msg.size() + 1, bytesSent))
		{
			std::cout << "[soc " << socket.GetId() << "][send " << bytesSent << "b] " << msg << std::endl;
			return true;
		}
		//std::cout << "[send " << bytesSent << "b fail]: " << msg << std::endl;
		return false;
	}

	void EchoServer()
	{
		NetSocket server(NetAddressType::IPv4);
		server.SetNonblockingMode();
		server.Bind(NetSocketIPv4Address(NetIPv4Address("0.0.0.0"), 3000));
		server.Listen();

		NetSocket client;
		while (!server.Accept(client)) {}

		while (true)
		{
			LogSend(client);
			LogReceive(client);
		}
	}

	const char* largeMessage =
		"Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown "
		"printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, "
		"remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop "
		"publishing software like Aldus PageMaker including versions of Lorem Ipsum.Contrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of "
		"classical Latin literature from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the "
		"more obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered the undoubtable source. "
		"Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of \"de Finibus Bonorum et Malorum\" (The Extremes of Good and Evil) by Cicero, written in 45 BC. This book is a "
		"treatise on the theory of ethics, very popular during the Renaissance. The first line of Lorem Ipsum, \"Lorem ipsum dolor sit amet..\", comes from a line in section "
		"1.10.32. [END]";

    const char message1KiB[1024] =
    "Mauris consectetur, lacus id porttitor maximus, ante tellus gravida enim, id ultrices ar arcu vitae lacus. in justo. Suspendisse ultricies sodales finibus. Vivamus vel dui mollis, hendrerit neque sed, elementum risus. Suspendisse ullamcorper justo a feugiat feugiat. Maecenas condimentum lectus sed vestibulum interdum. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Nulla lacus nunc, lacinia et quam in, commodo porttitor dolor. Aenean scelerisque velit ipsum, eget dapibus ligula bibendum vel. Mauris sit amet suscipit massa, vestibulum laoreet tellus. Sed volutpat tempus dictum. Nunc venenatis lacus vitae congue placerat. Quisque euismod ante nisi, vel accumsan elit auctor a. Vivamus sit amet ullamcorper nibh. Nam ut sagittis ex, sed varius tortor."
    "Praesent nulla enim, tempus vel volutpat lacinia, eleifend laoreet lacus. Sed mattis ultrices erat, a , quis imperdiet tellus viverra. Nulla cursus varius orci ut semper. Suspendisse id justo nunc. Duis semper pharetra laoreet.";

	void Client(int count = 1)
	{
		std::vector<NetSocket> clients;
		for (int i = 0; i < count; ++i)
		{
			auto& client = clients.emplace_back(NetAddressType::IPv4);
			client.Connect(NetSocketIPv4Address(NetIPv4Address("127.0.0.1"), 3000));
			client.SetNonblockingMode();
		}

        BufferPool<256> bufferPool;
        std::string msg;
		while (true)
		{
			for (auto& client : clients)
			{
                if (client.Receive(bufferPool, msg))
                {
                    std::cout << "[soc " << client.GetId() << "][recv " << msg.size() << "b] " << msg << std::endl;
                }

				//LogReceive(client);
				TimedTask<1>([&client]() { LogSend(client, largeMessage); });
				//TimedTask<2>([&client]() { LogSend(client, largeMessage); });
			}
		}
	}

	void PollServer()
	{
		NetTCPServer<NetPollHandler<NetTCPDataHandler>> server(NetSocketIPv4Address(NetIPv4Address("0.0.0.0"), 3000));
		server.OnConnection([](NetConnectionOld& connection) {
			std::cout << "[User] Connect: " << connection.GetId() << std::endl;

			connection.OnDisconnect([](const NetConnectionOld& connection) { std::cout << "[User] Disconnected: " << connection.GetId() << std::endl; });

			connection.OnMessage(
				[](const NetConnectionOld& connection, const std::string& msg) { std::cout << "[User] From: " << connection.GetId() << ", msg: " << msg << std::endl; });
		});
		while (true)
		{
			server.Accept();
			server.HandleNetEvents();
			TimedTask<5>([&server]() {
				std::cout << "Update()" << std::endl;
				server.Update();
			});
			TimedTask<3>([&server]() {
				if (server.connections.empty()) return;
				int rndClient = rand() % (server.connections.size());
				auto connection = server.connections.begin();
				std::advance(connection, rndClient);
				connection->messagesToSend.emplace("Hello from server");
			});
		}
	}

	void SelectServer()
	{
		NetTCPServer<NetSelectHandler<NetTCPDataHandler>> server(NetSocketIPv4Address(NetIPv4Address("0.0.0.0"), 3000));
		server.OnConnection([](NetConnectionOld& connection) {
			std::cout << "[User] Connect: " << connection.GetId() << std::endl;

			connection.OnDisconnect([](const NetConnectionOld& connection) { std::cout << "[User] Disconnected: " << connection.GetId() << std::endl; });

			connection.OnMessage(
				[](const NetConnectionOld& connection, const std::string& msg) { std::cout << "[User] From: " << connection.GetId() << ", msg: " << msg << std::endl; });
		});
		while (true)
		{
			server.Accept();
			server.HandleNetEvents();
			TimedTask<5>([&server]() {
				std::cout << "Update()" << std::endl;
				server.Update();
			});
			TimedTask<3>([&server]() {
				if (server.connections.empty()) return;
				int rndClient = rand() % (server.connections.size());
				auto connection = server.connections.begin();
				std::advance(connection, rndClient);
				connection->messagesToSend.emplace("Hello from server");
			});
		}
	}

	void IOCPServer()
	{
		NetTCPIOCPServer<NetIOCPManager<NetTCPAsyncDataHandler>> server(NetSocketIPv4Address(NetIPv4Address("0.0.0.0"), 3000));
		server.OnConnection([](NetConnection& connection) {
			std::cout << "[User] Connect: " << connection.GetId() << std::endl;

			connection.OnDisconnect([](const NetConnection& connection) { std::cout << "[User] Disconnected: " << connection.GetId() << std::endl; });

			connection.OnMessage([](const NetConnection& connection, const NetReceivedMessage& receivedMessage) {
				std::cout << "[User] From: " << connection.GetId() << ", msg: " << receivedMessage.msg << std::endl;
			});
		});
        bool close = false;
		while (!close)
		{
			server.Accept();
			server.HandleNetEvents();
			TimedTask<5>([&server]() {
				std::cout << "[User] Update()" << std::endl;
				server.Update();
			});
			TimedTask<3>([&server]() {
				if (server.connections.empty()) return;
				int rndClient = rand() % (server.connections.size());
				auto connection = server.connections.begin();
				std::advance(connection, rndClient);
				connection->messagesToSend.emplace("Hello from server");
                //connection->messagesToSend.emplace(largeMessage);
			});
            
            TimedTask<10>([&close]() {close = true;});
		}
	}
}
