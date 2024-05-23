#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <map>
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
		map<int, Player*> clients;

		struct addrinfo* result = NULL;
		struct addrinfo  hints;
		struct sockaddr_in servaddr, cliaddr;

		mutex mtx;
		thread* t_check_connections = nullptr;
		thread* t_listen_clientsTCP = nullptr;
		thread* t_listen_clientsUDP = nullptr;
		thread* t_send_clientsUDP   = nullptr;
		thread* t_move_players		= nullptr;
		void listen_clientsTCP();
		void listen_clientsUDP();
		void send_NEUDP();
		void move_players();
};

