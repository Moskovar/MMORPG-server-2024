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

    //t_check_connections = new thread(&Server::check_connections, this);
    //cout << "- Check connections thread runs !" << endl;

    t_listen_clientsTCP = new thread(&Server::listen_clientsTCP, this);
    cout << "- Listen TCP thread runs !" << endl;

    t_listen_clientsUDP = new thread(&Server::liste_clientsUDP, this);
    cout << "- Listen UDP thread runs !" << endl;

    //t_send_clientsUDP = new thread(&Server::send_clientsUDP, this);
    //cout << "- Listen UDP thread runs !" << endl;
}

Server::~Server()
{
    close();

    if (t_listen_clientsTCP->joinable()) t_listen_clientsTCP->join();

    for (Player* p : clients)
    {
        delete p;
        p = nullptr;
    }

    delete t_listen_clientsTCP;
    t_listen_clientsTCP = nullptr;
    cout << "Listen thread ended !" << endl;

}

void Server::close()
{
    run = false;

    for (Player* p : clients)
    {
        closesocket(*p->getTCPSocket());
        delete p;
        p = nullptr;
    }

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
            NetworkEntity ne = { clients.size(), 1920 + 800, 1080 + 500 };
            clients.push_back(new Player(tcpSocket, ne));
            clients[clients.size() - 1]->sendStructTCP(ne);

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
        for (Player* p : clients) 
        {
            if (*p->getTCPSocket() == INVALID_SOCKET) continue;
            ioctlsocket(*p->getTCPSocket(), FIONBIO, &mode);//mode non bloquant
            FD_SET(*p->getTCPSocket(), &readfds);
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
            SOCKET clientSocket = *(*it)->getTCPSocket();
            if (FD_ISSET(clientSocket, &readfds)) {
                //iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
                iResult = (*it)->recvTCP();
                if (iResult == 0) {
                    closesocket(clientSocket);
                    it = clients.erase(it);
                    std::cout << "A client has been disconnected, " << clients.size() << " left" << std::endl << endl;
                    continue;
                }
                else {
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

void Server::liste_clientsUDP()
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
            sockaddr_in from;
            int fromLen = sizeof(from);

            NetworkEntity ne;

            int bytesReceived = recvfrom(udpSocket, (char*)&ne, sizeof(ne), 0, (sockaddr*)&from, &fromLen);
            if (bytesReceived == SOCKET_ERROR) {
                std::cerr << "Erreur lors de la reception des donnees." << std::endl;
            }

            ne.id = htonl(ne.id);
            ne.x = htonl(ne.x);
            ne.y = htonl(ne.y);
            float x = ne.x;
            float y = ne.y;
            cout << ne.id << " : " << x / 100 << " : " << y / 100 << endl;
            
        }
    }  
}

/*int bytesReceived = recvfrom(udpSocket, recvbuf, recvbuflen, 0, (SOCKADDR*)&from, &fromLen);
            if (bytesReceived > 0) {
                std::cout << "Donnees: " << std::string(recvbuf, bytesReceived) << std::endl;
            }
            else if (bytesReceived == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    std::cerr << "recvfrom failed: " << error << std::endl;
                }
            }*/

void Server::send_clientsUDP()
{
    while (run)
    {
        //ne.id = htonl(ne.id);
        //ne.x = htonl(ne.x);
        //ne.y = htonl(ne.y);
        //char buffer[sizeof(ne)];
        //memcpy(buffer, &ne, sizeof(ne));

        //// Envoi des données sérialisées
        //int bytesSent = sendto(udpSocket, buffer, sizeof(ne), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        //if (bytesSent == SOCKET_ERROR) {
        //    std::cerr << "Erreur lors de l'envoi des données." << std::endl;
        //}
        //// Préparation du message à envoyer
        //const char* message = "Hello, clients!";
        //int messageLength = strlen(message) + 1;

        //mtx.lock();
        //// Envoi du message à chaque client
        //int bytesSent;
        //socklen_t len = sizeof(cliaddr);
        //bytesSent = sendto(udpSocket, message, messageLength, 0, (const struct sockaddr*)&cliaddr, len);
        //if (bytesSent == -1) {
        //    std::cerr << "Erreur lors de l'envoi du message." << std::endl;
        //    closesocket(udpSocket);
        //}
        //mtx.unlock();
    }
}

//char recvbuf[1024];
//int recvbuflen = 1024;
////std::cout << "Message envoyé avec succès." << std::endl;
//int clientAddrSize = sizeof(cliaddr);
//
//// Recevoir un message entrant
//int iResult = recvfrom(udpSocket, recvbuf, recvbuflen, 0, (sockaddr*)&cliaddr, &clientAddrSize);
//if (iResult == SOCKET_ERROR) {
//    std::cerr << "Erreur lors de la réception du message : " << WSAGetLastError() << std::endl;
//}
//else {
//    recvbuf[iResult] = '\0'; // Ajouter un caractère nul à la fin de la chaîne
//    std::cout << "Message reçu de " << " : " << recvbuf << std::endl;
//}
