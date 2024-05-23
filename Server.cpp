#include "Server.h"

Server::Server()//gérer les erreurs avec des exceptions
{
    // Initialiser Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        exit(1);
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
        exit(1);
    }

    // Créer un socket pour se connecter au serveur
    connectionSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (connectionSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        exit(1);
    }

    // Configurer le socket pour écouter
    iResult = bind(connectionSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(connectionSocket);
        WSACleanup();
        exit(1);
    }

    freeaddrinfo(result);

    iResult = ::listen(connectionSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(connectionSocket);
        WSACleanup();
        exit(1);
    }

    // Initialiser l'adresse du serveur
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8080);

    // Créer une socket UDP
    if ((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Lier la socket à l'adresse du serveur
    if (bind(udpSocket, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    u_long mode = 1;
    ioctlsocket(udpSocket, FIONBIO, &mode);

    t_listen_clientsTCP = new thread(&Server::listen_clientsTCP, this);
    cout << "- Listen TCP thread runs !" << endl;

    //t_listen_clientsUDP = new thread(&Server::listen_clientsUDP, this);
    //cout << "- Listen UDP thread runs !" << endl;
    
    t_send_clientsUDP   = new thread(&Server::send_NEUDP, this);
    cout << "- Send UDP thread runs !" << endl;

    t_move_players = new thread(&Server::move_players, this);
    cout << "- Move players thread runs !" << endl;
}

Server::~Server()
{
    close();

    if (t_listen_clientsTCP->joinable()) t_listen_clientsTCP->join();

    for (auto it = clients.begin(); it != clients.end(); ++it)
    {
        delete it->second;
        it->second = nullptr;
    }

    delete t_listen_clientsTCP;
    t_listen_clientsTCP = nullptr;
    cout << "Listen thread ended !" << endl;

}

void Server::close()
{
    run = false;

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
            int id = 0;
            for (int i = 0; i <= clients.size(); i++)//<= pour parcourir une case de plus et si tout est full alors le player sera placé à la fin (id = size)
            {
                if (clients[i] == nullptr) { id = i; break; }
            }
            NetworkEntity ne = { id, (1920 + 800) * 100, (1080 + 500) * 100 };
            clients[id] = new Player(tcpSocket, id, ne.xMap / 100, ne.yMap / 100);
            clients[id]->sendNETCP(ne);
            
            cout << "Connection succeed !" << endl << clients.size() << " clients connected !" << endl;
        }

        std::cout << "Waiting for new connection..." << std::endl;
    }
}

void Server::listen_clientsTCP()
{
    fd_set readfds;//structure pour surveiller un ensemble de descripteurs de fichiers pour lire (ici les sockets)
    timeval timeout;
    char recvbuf[512];
    int recvbuflen = 512;
    u_long mode = 1;

    while (run)
    {
        FD_ZERO(&readfds);//reset l'ensemble &readfds
        mtx.lock();
        //clients[]
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if (*it->second->getTCPSocket() == INVALID_SOCKET) continue;
            ioctlsocket(*it->second->getTCPSocket(), FIONBIO, &mode);//mode non bloquant
            FD_SET(*it->second->getTCPSocket(), &readfds);
        }
        mtx.unlock();

        // Vérifiez si le fd_set est vide
        if (readfds.fd_count == 0) {
            // Aucun socket à surveiller, attendez un peu avant de réessayer
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int iResult = select(0, &readfds, NULL, NULL, &timeout);//Attend que l'un des sockets dans readfds soit prêt pour la lecture (ou jusqu'à ce que le délai d'attente expire)
        if (iResult == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        mtx.lock();
        for (auto it = clients.begin(); it != clients.end();) {
            SOCKET clientSocket = *(*it).second->getTCPSocket();
            if (FD_ISSET(clientSocket, &readfds)) {
                //iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
                iResult = it->second->recvTCPShort();
                if (iResult == 0) {
                    closesocket(clientSocket);
                    it = clients.erase(it);
                    std::cout << "A client has been disconnected, " << clients.size() << " left" << std::endl << endl;
                    continue;
                }
                else if(iResult != sizeof(short)) {
                    std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
                    closesocket(clientSocket);
                    it = clients.erase(it);
                    std::cout << "A client has been disconnected, " << clients.size() << " left" << std::endl << endl;
                    continue;
                }
            }
            ++it;
        }
        mtx.unlock();
        Sleep(1);
    }
}

void Server::listen_clientsUDP()
{
    cout << "UDP LISTENING" << endl;

    fd_set readfds;
    timeval timeout;
    char recvbuf[512];
    int recvbuflen = 512;

    while (run)
    {
        FD_ZERO(&readfds);
        FD_SET(udpSocket, &readfds);

        // Vérifiez si le fd_set est vide
        if (readfds.fd_count == 0) {
            // Aucun socket à surveiller, attendez un peu avant de réessayer
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        timeout.tv_sec = 1;  // 1 seconde de timeout
        timeout.tv_usec = 0;

        int iResult = select(0, &readfds, NULL, NULL, &timeout);//Attend que l'un des sockets dans readfds soit prêt pour la lecture (ou jusqu'à ce que le délai d'attente expire)
        if (iResult == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        if (FD_ISSET(udpSocket, &readfds)) {
            sockaddr_in clientAddr;
            int clientAddrLen = sizeof(clientAddr);

            NetworkEntity ne;

            int bytesReceived = recvfrom(udpSocket, (char*)&ne, sizeof(ne), 0, (sockaddr*)&clientAddr, &clientAddrLen);
            if (bytesReceived == SOCKET_ERROR) {
                std::cerr << "Erreur lors de la reception des donnees." << std::endl;
            }

            //std::cout << "Received message from " << clientIp << endl;

            ne.id = htonl(ne.id);
            ne.xMap = htonl(ne.xMap);
            ne.yMap = htonl(ne.yMap);
            
            float xMap = ne.xMap / 100;
            float yMap = ne.yMap / 100;

            mtx.lock();
            clients[ne.id]->setAddr(clientAddr);
            clients[ne.id]->setPos(xMap, yMap);

            sockaddr_in cli = *clients[ne.id]->getPAddr();
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(cli.sin_addr), clientIp, INET_ADDRSTRLEN);
            cout << clients[ne.id]->getID() << " : " << clients[ne.id]->getXMap() << " : " << clients[ne.id]->getYMap() << " : " << clientIp << endl;

            mtx.unlock();
        }
    }  
}

void Server::send_NEUDP()
{
    NetworkEntity ne = { 0, 250, 250 };
    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(8080); // Utiliser le port d'écoute du client
    inet_pton(AF_INET, "127.0.0.1", &clientAddr.sin_addr);
    while (run)
    {
        ne.id = htonl(ne.id);
        ne.xMap = htonl(ne.xMap);
        ne.yMap = htonl(ne.yMap);
        char buffer[sizeof(ne)];
        memcpy(buffer, &ne, sizeof(ne));
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if (it->second == nullptr) continue;
            ne.id = it->second->getID();
            ne.xMap  = it->second->getXMap() * 100;
            ne.yMap  = it->second->getYMap() * 100;
            // Envoi des données sérialisées
            int bytesSent = sendto(udpSocket, buffer, sizeof(ne), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
            if (bytesSent == SOCKET_ERROR) {
                std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
            }
        }
        Sleep(1);
    }
}

void Server::move_players()
{
    while (true)
    {
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if(it->second != nullptr) it->second->move();
            //cout << it->second->getXMap() << " : " << it->second->getYMap() << endl;
        }
        Sleep(30);
    }
}
