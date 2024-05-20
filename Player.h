#pragma once
#include <winsock2.h>
#include <iostream>
//#include <ws2tcpip.h>

using namespace std;

class Player
{
public:
	Player(SOCKET* tcpSocket, SOCKET* udpSocket);
	~Player();

	bool isSocketValid() { return !(*tcpSocket == INVALID_SOCKET); }
	SOCKET* getTCPSocket() { return this->tcpSocket; }

	void sendTCP(const char* sendbuf);
	void sendUDP(const char* sendbuf);
	bool recvTCP();

private:
	SOCKET* tcpSocket = nullptr;
	SOCKET* udpSocket = nullptr;
};

