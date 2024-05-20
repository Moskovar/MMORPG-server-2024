#include "Server.h"

Server::Server()//gérer les erreurs avec des exceptions
{
    // Initialiser Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Résoudre l'adresse et le port du serveur
    iResult = getaddrinfo(NULL, "27015", &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
    }

    // Créer un socket pour se connecter au serveur
    connectionSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (connectionSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
    }

    // Configurer le socket pour écouter
    iResult = bind(connectionSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(connectionSocket);
        WSACleanup();
    }

    freeaddrinfo(result);

    iResult = ::listen(connectionSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(connectionSocket);
        WSACleanup();
    }

    t_listen_clients = new thread(&Server::listen_clients, this);
    cout << "Listen thread runs !" << endl;

    //t_check_clients = new thread(&Server::check_clients, this);
    //cout << "Check thread runs !" << endl;


    /*udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la création du socket UDP." << std::endl;
        WSACleanup();
    }
    
    // Configuration de l'adresse et du port du serveur
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Écoute sur toutes les interfaces
    serverAddr.sin_port = htons(12345); // Port du serveur

    // Lier le socket à l'adresse et au port
    if (bind(udpSocket, (const sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors de la liaison du socket à l'adresse et au port." << std::endl;
        closesocket(udpSocket);
        WSACleanup();
    }

    std::cout << "Serveur UDP demarre. En attente de donnees..." << std::endl;

    // Réception des données du client
    char recvBuf[1024];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int bytesReceived = recvfrom(udpSocket, recvBuf, sizeof(recvBuf), 0, (sockaddr*)&clientAddr, &clientAddrLen);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Erreur lors de la reception des donnees du client." << std::endl;
    }
    else {
        std::cout << "Donnees reçues du client: " << std::string(recvBuf, bytesReceived) << std::endl;
    }

    // Fermeture du socket et libération de Winsock
    closesocket(udpSocket);*/
}

Server::~Server()
{
    close();

    if (t_listen_clients->joinable()) t_listen_clients->join();
    delete t_listen_clients;
    t_listen_clients = nullptr;
    cout << "Listen thread ended !" << endl;

    //if (t_check_clients->joinable()) t_check_clients->join();
    //delete t_check_clients;
    //t_check_clients = nullptr;

    cout << "Check thread ended !" << endl;

}

void Server::close()
{
    run = false;

    /*for (Player p : clients)
    {
        closesocket(*s);
        delete s;
        s = nullptr;
    }*/

    closesocket(connectionSocket);
    WSACleanup();

    printf("Server closed now !");
}

void Server::listen()
{
    std::cout << "Waiting for connections..." << std::endl;
    while (run)
    {
        SOCKET* tcpSocket = new SOCKET();
        *tcpSocket = accept(connectionSocket, NULL, NULL);
        if (*tcpSocket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            closesocket(connectionSocket);
            WSACleanup();
        }
        else
        {
            mtx.lock();
            clients.push_back(new Player(tcpSocket, nullptr));
            cout << "Connection succeed !" << endl << clients.size() << " clients connected !" << endl;
            mtx.unlock();
        }

        // Fermeture de la connexion
        /*iResult = shutdown(clientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "shutdown failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
        }*/

        std::cout << "Waiting for new connection..." << std::endl;
    }
}

void Server::check_clients()
{
    /*while (run)
    {
        mtx.lock();
        for (int i = clients.size() - 1; i >= 0; i--)
        {
            if (clients[i] == nullptr)
            {
                clients.erase(clients.begin() + i);
                cout << "A client has been disconnected !" << endl << clients.size() << " still connected !" << endl;
            }
            cout << clients[i] << endl;
        }
        mtx.unlock();
        Sleep(2000);
    }*/
}

void Server::listen_clients()
{
    while (run)
    {
        mtx.lock();
        for (int i = clients.size() - 1; i >= 0; i--)
        {
            //if (*clients[i] == INVALID_SOCKET) continue;
            if (!clients[i]->recvTCP())
            {
                if (clients[i] != nullptr)
                {
                    delete clients[i];
                    clients[i] == nullptr;
                }
                clients.erase(clients.begin() + i);
                cout << "A player has been disconnected, " << clients.size() << " left" << endl;
            }
        }
        mtx.unlock();
    }
}
