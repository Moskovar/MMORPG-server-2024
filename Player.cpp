#include "Player.h"

Player::Player(SOCKET* tcpSocket, NetworkEntity networkEntity)
{
	this->tcpSocket = tcpSocket;
	this->networkEntity = networkEntity;
}

Player::~Player()
{
	if (tcpSocket != nullptr)
	{
		closesocket(*tcpSocket);
		delete tcpSocket;
		tcpSocket = nullptr;
	}

    cout << "Player cleared !" << endl;
}

void Player::sendNETCP(NetworkEntity& ne)
{
	// Envoyer une réponse
	if (tcpSocket != nullptr)
	{
		ne.id = htonl(ne.id);
		ne.x = htons(ne.x);
		ne.y = htons(ne.y);
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
