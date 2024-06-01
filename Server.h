#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include "Player.h"


#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define MAX_PLAYER_NUMBER 100

class Server
{
	public:
		Server();
		~Server();

		void accept_connections();

	private:
		bool run = true, isDeleting = false;
		WSADATA wsaData;

		SOCKET connectionSocket = INVALID_SOCKET;//socket pour recevoir et accepter les connexions des clients
		SOCKET udpSocket		= INVALID_SOCKET;//socket pour communiquer en UDP
		map<int, Player*> players;//liste dans laquelle sont plac�s les nouveaux joueurs dont la connexion a �t� accept�e

		sockaddr_in udpServerAddr, tcpServerAddr;//addesse du socket de connexion et du socket udp

		mutex mtx, mtx_sendNETCP;//mutex pour g�rer l'acc�s aux ressources 
		thread* t_listen_clientsTCP = nullptr;//thread pour �couter les joueurs en TCP
		thread* t_listen_clientsUDP = nullptr;//thread pour �couter les joueurs en UDP
		thread* t_send_clientsTCP   = nullptr;//thread pour envoyer des donn�es en TCP aux joueurs
		thread* t_send_clientsUDP   = nullptr;//thread pour envoyer des donn�es en UDP aux joueurs
		thread* t_move_players		= nullptr;
		void listen_clientsTCP();
		void listen_clientsUDP();
		bool recv_NEUDP(NetworkEntity& ne, sockaddr_in clientAddr);
		void send_NEUDP();
		void send_NETCP(NetworkEntity ne, Player* p);
		void send_NESTCP(NetworkEntitySpell nes, Player* p);
};

