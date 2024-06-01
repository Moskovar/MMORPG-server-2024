#pragma once
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <chrono>
//#include <ws2tcpip.h>

using namespace std;

#pragma pack(push, 1)
struct NetworkEntity {
	short header = 0;
	short id = 0, countDir = 0, hp = 0;
	int   xMap = 0, yMap = 0;
	uint64_t timestamp; // En microsecondes depuis l'epoch
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NetworkEntitySpell {
	short header = 1;
	short id = 0, spellID = 0;
};
#pragma pack(pop)

//uint64_t getCurrentTimestamp() {
//	auto now = std::chrono::steady_clock::now();
//	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
//	return duration.count();
//}

class Player
{
public:
	Player(SOCKET* tcpSocket, short id, short hp, float xMap, float yMap);
	~Player();

	bool isSocketValid() { return tcpSocket && *tcpSocket != INVALID_SOCKET; }//enlever le pointeur socket ??
	SOCKET* getTCPSocket() { return this->tcpSocket; }
	sockaddr_in* getPAddr() { return &addr; }
	int getID() { return id; }
	int getAddrLen() { return addrLen; }
	float getXMap() { return xMap; }
	float getYMap() { return yMap; }
	NetworkEntity getNE() { return { 0, id, countDir, hp, (int)xMap*100, (int)yMap*100, 0 }; }
	
	//--- joueur ---//
	void setPos(float xMap, float yMap) { this->xMap = xMap; this->yMap = yMap; }
	void update(NetworkEntity& ne);
	void move();

	//--- communication ---//
	void setAddr(sockaddr_in addr) { this->addr = addr; addrLen = sizeof(this->addr); }
	void sendNETCP(NetworkEntity ne);
	void sendNESTCP(NetworkEntitySpell nes);
	int recvTCP();
	int recvTCPShort();
	void setNE(NetworkEntity& ne) { ne.id = id; ne.countDir = countDir; ne.xMap = xMap * 100; ne.yMap = yMap * 100; }

	bool connected = true;
	vector<char> recvBuffer;

	short spell = 0;

private:
	SOCKET* tcpSocket = nullptr;
	sockaddr_in addr = {};
	short id	   = 0    , hp   = 0;
	int	  addrLen  = 0;
	short countDir = 0	  ,	dir  = 0;
	float xMap	   = 0.0f , yMap = 0.0f, xRate = 0.0f, yRate = 0.0f, xChange = 0.0f, yChange = 0.0f, speed = 400;

	std::chrono::steady_clock::time_point prevTime;//pour calculer le deltatime
};

