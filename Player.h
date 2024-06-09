#pragma once
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <chrono>
#include "uti.h"
//#include <ws2tcpip.h>

using namespace std;

class Player
{
public:
	Player(SOCKET* tcpSocket, short id, short hp, float xMap, float yMap);
	~Player();

	//--- Données du joueur ---//
	short getID() { return id; }
	int getAddrLen() { return addrLen; }
	float getXMap() { return xMap; }
	float getYMap() { return yMap; }
	short getFaction() { return this->faction; }
	
	void setPos(float xMap, float yMap) { this->xMap = xMap; this->yMap = yMap; }
	void setFaction(short faction) { this->faction = faction; }
	void applyDmg(short dmg);

	//--- Lien entre la communication et le joueur ---//
	void update(uti::NetworkEntity& ne);

	//--- communication ---//
	SOCKET* getTCPSocket() { return this->tcpSocket; }
	sockaddr_in* getPAddr() { return &addr; }
	bool isSocketValid() { return tcpSocket && *tcpSocket != INVALID_SOCKET; }
	uti::NetworkEntity getNE() { return { uti::Header::NE, id, countDir, hp, (int)(xMap * 100), (int)(yMap * 100), 0 }; }

	void setAddr(sockaddr_in addr) { this->addr = addr; addrLen = sizeof(this->addr); }
	void sendNETCP (uti::NetworkEntity ne);
	void sendNESTCP(uti::NetworkEntitySpell nes);
	void sendNESETCP(uti::NetworkEntitySpellEffect nese);
	void sendNEFTCP(uti::NetworkEntityFaction nef);

	bool connected = true;
	vector<char> recvBuffer;

private:
	SOCKET* tcpSocket = nullptr;
	sockaddr_in addr = {};
	int	  addrLen  = 0;
	short id	   = 0    , countDir = 0   , hp         = 0   , faction    = 0   , spell = 0   , dir = 0;;
	float xMap	   = 0.0f , yMap     = 0.0f, xCenterBox = 0.0f, yCenterBox = 0.0f, xRate = 0.0f, yRate = 0.0f, xChange = 0.0f, yChange = 0.0f, speed = 400;

	std::chrono::steady_clock::time_point prevTime;//pour calculer le deltatime
};

