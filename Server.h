#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <ctime>
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
		map<int, Player*> players;//liste dans laquelle sont placés les nouveaux joueurs dont la connexion a été acceptée

		sockaddr_in udpServerAddr, tcpServerAddr;//addesse du socket de connexion et du socket udp

		mutex mtx, mtx_sendNETCP;//mutex pour gérer l'accès aux ressources 
		thread* t_listen_clientsTCP = nullptr;//thread pour écouter les joueurs en TCP
		thread* t_listen_clientsUDP = nullptr;//thread pour écouter les joueurs en UDP
		thread* t_send_clientsTCP   = nullptr;//thread pour envoyer des données en TCP aux joueurs
		thread* t_send_clientsUDP   = nullptr;//thread pour envoyer des données en UDP aux joueurs
		thread* t_move_players		= nullptr;
		void listen_clientsTCP();
		void listen_clientsUDP();
		void send_NETCP(uti::NetworkEntity ne, Player* p);//envoie à tlm sauf le joueur en paramètre
		void send_NETCP(uti::NetworkEntity ne);//envoie à tlm
		void send_NESTCP(uti::NetworkEntitySpell nes, Player* p);
		void send_NESETCP(uti::NetworkEntitySpellEffect nese, Player* p);
		void send_NEFTCP(uti::NetworkEntityFaction nef, Player* p);

		bool recv_NEUDP(uti::NetworkEntity& ne, sockaddr_in clientAddr);
		void send_NEUDP();
};

