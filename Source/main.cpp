// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#include "Servers.hpp"

#pragma comment(lib, "Ws2_32.lib")

char* getCmdOption(char** begin, char** end, const std::string& option)
{
	char** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end) { return *itr; }
	return nullptr;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}

int main(int argc, char* argv[])
{

	/////////////////////////////

	// Server(1) or Client(2)
	int option = 1;

	// Number of clients
	int clientCount = 1;

	// Server type PollServer(1) or SelectServer(2) or IOCPServer(3)
	int serverTypeOption = 3;

	/////////////////////////////

	setlocale(LC_ALL, "Russian");
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData) != NO_ERROR)
	{
		std::cout << "Error WinSock version initialization #";
		std::cout << WSAGetLastError();
		return 1;
	}
	else { std::cout << "WinSock initialization is OK" << std::endl; }

	if (cmdOptionExists(argv, argv + argc, "-s") || cmdOptionExists(argv, argv + argc, "--server")) { option = 1; }
	else if (cmdOptionExists(argv, argv + argc, "-c"))
	{
		option = 2;

		char* clientCountStr = getCmdOption(argv, argv + argc, "-c");
		if (clientCountStr != nullptr)
		{
			clientCount = std::stoi(clientCountStr);
			if (clientCount < 1) clientCount = 1;
		}
	}
	else if (cmdOptionExists(argv, argv + argc, "--client"))
	{
		option = 2;

		char* clientCountStr = getCmdOption(argv, argv + argc, "-client");
		if (clientCountStr != nullptr)
		{
			clientCount = std::stoi(clientCountStr);
			if (clientCount < 1) clientCount = 1;
		}
	}



	if (cmdOptionExists(argv, argv + argc, "--poll")) { serverTypeOption = 1; }
	else if (cmdOptionExists(argv, argv + argc, "--select")) { serverTypeOption = 2; }
	else if (cmdOptionExists(argv, argv + argc, "--iocp")) { serverTypeOption = 3; }

	//    char* filename = getCmdOption(argv, argv + argc, "-f");
	//    if (filename)
	//    {
	//    }

	while (true)
	{
		if (option == 0)
		{
			std::cout << "1 - Server" << std::endl;
			std::cout << "2 - Client" << std::endl;
			std::cin >> option;
		}

		if (option == 1)
		{
			switch (serverTypeOption)
			{
				case 1: LimeEngine::Net::EchoServer::PollServer(); break;
				case 2: LimeEngine::Net::EchoServer::SelectServer(); break;
				case 3: LimeEngine::Net::EchoServer::IOCPServer(); break;

				default: LimeEngine::Net::EchoServer::IOCPServer(); break;
			}
			break;
		}
		else if (option == 2)
		{
			std::cout << "Run " << clientCount << " Clients" << std::endl;
			LimeEngine::Net::EchoServer::Client(clientCount);
			break;
		}
		else if (option == 0) { break; }
	}

	WSACleanup();

	std::cout << "Exit" << std::endl;
	std::cout << "Press any button..." << std::endl;
	int q = std::cin.get();
	return 0;
}
