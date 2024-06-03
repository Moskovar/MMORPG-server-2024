#include "Player.h"

Player::Player(SOCKET* tcpSocket, short id, short hp, float xMap, float yMap)
{
	this->tcpSocket = tcpSocket;
	this->id		= id;
    this->hp        = hp;
	this->xMap		= xMap;
	this->yMap		= yMap;

    //Socket mit en mode non bloquant
    u_long mode = 1;
    ioctlsocket(*tcpSocket, FIONBIO, &mode);//mode non bloquant
}

Player::~Player()
{
	if (tcpSocket != nullptr)
	{
		closesocket(*tcpSocket);
		delete tcpSocket;
		tcpSocket = nullptr;
	}

    cout << "Player cleared !\n" << endl;
}

void Player::update(uti::NetworkEntity& ne)
{
    this->countDir = countDir;
    switch (this->countDir)
    {
        case 0:  xRate = 0;    yRate = 0;               break;
        case 1:  xRate = 0;    yRate = -1;   dir = 0;   break;
        case 3:  xRate = 1;    yRate = 0;    dir = 1;   break;
        case 6:  xRate = 0;    yRate = 1;    dir = 2;   break;
        case 11: xRate = -1;   yRate = 0;    dir = 3;   break;
        case 4:  xRate = 0.5;  yRate = -0.5; dir = 0.5; break;
        case 7:  xRate = 0;    yRate = 0;               break;
        case 12: xRate = -0.5; yRate = -0.5; dir = 3.5; break;
        case 9:  xRate = 0.5;  yRate = 0.5;  dir = 1.5; break;
        case 14: xRate = 0;    yRate = 0;               break;
        case 17: xRate = -0.5; yRate = 0.5;  dir = 2.5; break;
        case 10: xRate = 1;    yRate = 0;    dir = 1;   break;
        case 15: xRate = 0;    yRate = -1;   dir = 0;   break;
        case 18: xRate = -1;   yRate = 0;    dir = 3;   break;
        case 20: xRate = 0;    yRate = 1;    dir = 2;   break;
        case 21: xRate = 0;    yRate = 0;               break;
        default: xRate = 0;    yRate = 0;               break;
    }
    xMap  = (float)ne.xMap / 100;
    yMap  = (float)ne.yMap / 100;
    hp    = ne.hp;
    //spell = ne.spell;

    //cout << "Player ID: " << ne.id << " position is now: " << xMap << " : " << yMap << " -> countDir: " << countDir << endl;
}

void Player::move()
{
}

void Player::sendNETCP(uti::NetworkEntity ne)
{
	// Envoyer une réponse
	if (tcpSocket != nullptr)
	{
        cout << "SEND NE FROM TCP: " << ne.id << " : " << ne.hp << " : " << ne.countDir << " : " << ne.xMap << " : " << ne.yMap << endl;
        ne.header   = htons(ne.header);
		ne.id       = htons(ne.id);
        ne.hp       = htons(ne.hp);
        ne.countDir = htons(ne.countDir);
		ne.xMap     = htonl(ne.xMap);
		ne.yMap     = htonl(ne.yMap);
        //ne.spell    = htons(ne.spell);
		int iResult = ::send(*tcpSocket, reinterpret_cast<const char*>(&ne), sizeof(ne), 0);
		if (iResult == SOCKET_ERROR) {
			std::cerr << "send failed: " << WSAGetLastError() << std::endl;
			closesocket(*tcpSocket);
		}
	}
}

void Player::sendNESTCP(uti::NetworkEntitySpell nes)
{
    // Envoyer une réponse
    if (tcpSocket != nullptr)
    {
        cout << "SEND NES FROM TCP: " << nes.header << " : " << nes.id << " : " << nes.spellID << endl;
        nes.header  = htons(nes.header);
        nes.id      = htons(nes.id);
        nes.spellID = htons(nes.spellID);

        int iResult = ::send(*tcpSocket, reinterpret_cast<const char*>(&nes), sizeof(nes), 0);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(*tcpSocket);
        }
    }
}

void Player::sendNESETCP(uti::NetworkEntitySpellEffect nese)
{
    // Envoyer une réponse
    if (tcpSocket != nullptr)
    {
        cout << "SEND NESE FROM TCP: " << nese.header << " : " << nese.id << " : " << nese.spellID << endl;
        nese.header  = htons(nese.header);
        nese.id      = htons(nese.id);
        nese.spellID = htons(nese.spellID);

        int iResult = ::send(*tcpSocket, reinterpret_cast<const char*>(&nese), sizeof(nese), 0);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(*tcpSocket);
        }
    }
}

void Player::sendNEFTCP(uti::NetworkEntityFaction nef)
{
    // Envoyer une réponse
    if (tcpSocket != nullptr)
    {
        cout << "SEND NES FROM TCP: " << nef.header << " : " << nef.id << " : " << nef.faction << endl;
        nef.header  = htons(nef.header);
        nef.id      = htons(nef.id);
        nef.faction = htons(nef.faction);

        int iResult = ::send(*tcpSocket, reinterpret_cast<const char*>(&nef), sizeof(nef), 0);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(*tcpSocket);
        }
    }
}

int Player::recvTCPShort()
{
    int iResult = 0;
    short data;
    // Recevoir des données
    if (tcpSocket != nullptr)
    {
        iResult = ::recv(*tcpSocket, (char*)&data, sizeof(data), 0);
        if (iResult == sizeof(data)) {
            // Conversion de l'entier de l'ordre du réseau à l'ordre de l'hôte
            short dataReceived = ntohs(data);
            //cout << "TCP short received: " << dataReceived << endl;
            //this->countDir = dataReceived;
        }
    }

    //cout << "iResult: " << iResult << endl;
    return iResult;
}
