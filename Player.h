#pragma once
#include <winsock2.h>
#include <iostream>
//#include <ws2tcpip.h>

using namespace std;

struct NetworkEntity {
	int id = 0, xMap = 0, yMap = 0;
};

class Player
{
public:
	Player(SOCKET* tcpSocket, int id, float xMap, float yMap);
	~Player();

	bool isSocketValid() { return !(*tcpSocket == INVALID_SOCKET); }
	SOCKET* getTCPSocket() { return this->tcpSocket; }
	sockaddr_in* getPAddr() { return &addr; }
	int getID() { return id; }
	int getAddrLen() { return addrLen; }
	float getXMap() { return xMap; }
	float getYMap() { return yMap; }
	
	void updateDir();
	void move();
	void setCountDir(short countDir) { this->countDir = countDir; }
	void setPos(float xMap, float yMap) { this->xMap = xMap; this->yMap = yMap; }
	void setAddr(sockaddr_in addr) { this->addr = addr; addrLen = sizeof(this->addr); }
	void sendNETCP(NetworkEntity& ne);
	int recvTCP();
	int recvTCPShort();

private:
	SOCKET* tcpSocket = nullptr;
	sockaddr_in addr;
	int id, addrLen;
	short countDir = 0, dir = 2;
	float xMap, yMap, xRate, yRate, xChange, yChange, speed = 400;
};

