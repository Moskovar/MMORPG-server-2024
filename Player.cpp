#include "Player.h"

Player::Player(SOCKET* tcpSocket, int id, float xMap, float yMap)
{
	this->tcpSocket = tcpSocket;
	this->id		= id;
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

void Player::update(NetworkEntity& ne)
{
    switch (ne.countDir)
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
    xMap = (float)ne.xMap / 100;
    yMap = (float)ne.yMap / 100;

    cout << "Player ID: " << ne.id << " position is now: " << xMap << " : " << yMap << endl;
}

void Player::move()
{
}

void Player::sendNETCP(NetworkEntity ne)
{
	// Envoyer une réponse
	if (tcpSocket != nullptr)
	{
        cout << "SEND NE FROM TCP: " << ne.id << " : " << ne.xMap << " : " << ne.yMap << endl;
		ne.id       = htons(ne.id);
		ne.xMap     = htonl(ne.xMap);
		ne.yMap     = htonl(ne.yMap);
		int iResult = ::send(*tcpSocket, reinterpret_cast<const char*>(&ne), sizeof(ne), 0);
		if (iResult == SOCKET_ERROR) {
			std::cerr << "send failed: " << WSAGetLastError() << std::endl;
			closesocket(*tcpSocket);
		}
	}
}

int Player::recvTCP()
{
    char recvbuf[1024];
    int recvbuflen = 1024;
	int iResult = 0;
    // Recevoir des données
    if (tcpSocket != nullptr)
    {
        iResult = ::recv(*tcpSocket, recvbuf, recvbuflen - 1, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0'; // Terminer la chaîne avec un caractère nul
            //std::cout << "Bytes received: " << iResult << std::endl;
            std::cout << "Received: " << recvbuf << std::endl;
        }
    }
    return iResult;
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
