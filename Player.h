#pragma once
#include <winsock2.h>
#include <iostream>
//#include <ws2tcpip.h>

using namespace std;

struct NetworkEntity {
	int id = 0, x = 0, y = 0;
};

class Player
{
public:
	Player(SOCKET* tcpSocket, NetworkEntity networkEntity);
	~Player();

	bool isSocketValid() { return !(*tcpSocket == INVALID_SOCKET); }
	SOCKET* getTCPSocket() { return this->tcpSocket; }

	void sendStructTCP(NetworkEntity& ne);
	int recvTCP();

private:
	SOCKET* tcpSocket = nullptr;
	NetworkEntity networkEntity;
};

