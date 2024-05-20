#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include "Player.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

class Server
{
	public:
		Server();
		~Server();

		void close();
		void listen();

	private:
		bool run = true;
		WSADATA wsaData;

		SOCKET connectionSocket = INVALID_SOCKET;
		SOCKET udpSocket		= INVALID_SOCKET;
		vector<Player*> clients;

		struct addrinfo* result = NULL;
		struct addrinfo  hints;

		mutex mtx;
		thread* t_check_clients  = nullptr;
		void check_clients();

		thread* t_listen_clients = nullptr;
		void listen_clients();
};

