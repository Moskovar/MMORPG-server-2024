#include "Player.h"

Player::Player(SOCKET* tcpSocket, SOCKET* udpSocket)
{
	this->tcpSocket = tcpSocket;
	this->udpSocket = udpSocket;
}

Player::~Player()
{
	if (tcpSocket != nullptr)
	{
		closesocket(*tcpSocket);
		delete tcpSocket;
		tcpSocket = nullptr;
	}

	if (udpSocket != nullptr)
	{
		closesocket(*udpSocket);
		delete udpSocket;
		udpSocket = nullptr;
	}

    cout << "Player cleared !" << endl;
}

void Player::sendTCP(const char* sendbuf)
{
	// Envoyer une réponse
	if (tcpSocket != nullptr)
	{
		int iResult = ::send(*tcpSocket, sendbuf, (int)strlen(sendbuf), 0);
		if (iResult == SOCKET_ERROR) {
			std::cerr << "send failed: " << WSAGetLastError() << std::endl;
			closesocket(*tcpSocket);
		}
	}
}

void Player::sendUDP(const char* sendbuf)
{
    // Réception des données du client
    char recvBuf[1024];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int bytesReceived = recvfrom(*udpSocket, recvBuf, sizeof(recvBuf), 0, (sockaddr*)&clientAddr, &clientAddrLen);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Erreur lors de la reception des donnees du client." << std::endl;
    }
    else {
        std::cout << "Donnees reçues du client: " << std::string(recvBuf, bytesReceived) << std::endl;
    }
}

bool Player::recvTCP()
{
    char recvbuf[1024];
    int recvbuflen = 1024;
    // Recevoir des données
    if (tcpSocket != nullptr)
    {
        int iResult = ::recv(*tcpSocket, recvbuf, recvbuflen - 1, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0'; // Terminer la chaîne avec un caractère nul
            //std::cout << "Bytes received: " << iResult << std::endl;
            std::cout << "Received: " << recvbuf << std::endl;
        }
        else if (iResult == 0) {
            //std::cout << "Connection closing..." << std::endl;
            closesocket(*tcpSocket);
            return false;
        }
        else {
            int err = WSAGetLastError();
            if (err != 10038 && err != 10054) std::cout << "recv failed: " << err << std::endl;
            closesocket(*tcpSocket);
            return false;
        }
    }
    return true;
}
