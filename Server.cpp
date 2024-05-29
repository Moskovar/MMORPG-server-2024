#include "Server.h"
#include <ctime>

Server::Server()//gérer les erreurs avec des exceptions
{
    // Initialiser Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        exit(1);
    }

    //--- TCP SOCKET ---//
    // Initialisation du socket TCP
    connectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectionSocket == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la création du socket TCP" << std::endl;
        WSACleanup();
        exit(1);
    }

    //Initialisation de l'adresse et du port d'écoute du serveur TCP
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_port = htons(9090);
    tcpServerAddr.sin_addr.s_addr = INADDR_ANY;

    //Bind du socket TCP à son adresse et port d'écoute
    if (bind(connectionSocket, (sockaddr*)&tcpServerAddr, sizeof(tcpServerAddr)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors du bind du socket TCP" << std::endl;
        closesocket(connectionSocket);
        WSACleanup();
        exit(1);
    }

    cout << "Socket TCP a l'ecoute sur le port 9090" << endl;

    //Mise à l'écoute du socket TCP
    if (listen(connectionSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Erreur lors de l'écoute sur le socket TCP." << std::endl;
        closesocket(connectionSocket);
        WSACleanup();
        exit(1);
    }

    //--- UDP SOCKET ---//
    // Initialisation du socket UDP
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la création du socket UDP." << std::endl;
        closesocket(connectionSocket);
        WSACleanup();
        exit(1);
    }

    //Définition de l'adresse et du port d'écoute du socket UDP
    udpServerAddr.sin_family = AF_INET;
    udpServerAddr.sin_port = htons(8080);
    udpServerAddr.sin_addr.s_addr = INADDR_ANY;

    //Bind du socket UDP à son adresse et port d'écoute
    if (bind(udpSocket, (const sockaddr*)&udpServerAddr, sizeof(udpServerAddr)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors du bind du socket UDP." << std::endl;
        closesocket(connectionSocket);
        closesocket(udpSocket);
        WSACleanup();
        exit(1);
    }
    cout << "Socket UDP a l'ecoute sur le port 8080" << endl;

    //Mise à l'état non bloquant du socket UDP
    u_long mode = 1;
    ioctlsocket(udpSocket, FIONBIO, &mode);

    //t_check_connections = new thread(&Server::check_connections, this);
    //cout << "- Check connections thread runs !" << endl;

    t_listen_clientsTCP = new thread(&Server::listen_clientsTCP, this);
    cout << "- Listen TCP thread runs !" << endl;

    //t_listen_clientsUDP = new thread(&Server::listen_clientsUDP, this);
    //cout << "- Listen UDP thread runs !" << endl;
    
    //t_send_clientsUDP   = new thread(&Server::send_NEUDP, this);
    //cout << "- Send UDP thread runs !" << endl;

    //t_move_players = new thread(&Server::move_players, this);
    //cout << "- Move players thread runs !" << endl;
}

Server::~Server()
{
    if (t_listen_clientsTCP != nullptr && t_listen_clientsTCP->joinable()) t_listen_clientsTCP->join(); 
    if (t_listen_clientsUDP != nullptr && t_listen_clientsUDP->joinable()) t_listen_clientsUDP->join();
    if (t_send_clientsTCP   != nullptr && t_send_clientsTCP->joinable())   t_send_clientsTCP->join();
    if (t_send_clientsUDP   != nullptr && t_send_clientsUDP->joinable())   t_send_clientsUDP->join();
    if (t_move_players      != nullptr && t_move_players->joinable())      t_move_players->join();

    delete t_listen_clientsTCP;
    delete t_listen_clientsUDP;
    delete t_send_clientsTCP;
    delete t_send_clientsUDP;
    delete t_move_players;

    t_listen_clientsTCP = nullptr;
    t_listen_clientsUDP = nullptr;
    t_send_clientsTCP   = nullptr;
    t_send_clientsUDP   = nullptr;
    t_move_players      = nullptr;

    for (auto it = players.begin(); it != players.end();)
    {
        delete it->second;
        it->second = nullptr;
        it = players.erase(it);
    }

    closesocket(connectionSocket);
    closesocket(udpSocket);
    WSACleanup();

    cout << "Le serveur est complétement nettoyé" << endl;
}

void Server::accept_connections()
{
    std::cout << "Waiting for connections..." << std::endl;
    while (run)
    {
        SOCKET* tcpSocket = new SOCKET();
        *tcpSocket = accept(connectionSocket, NULL, NULL);
        if (*tcpSocket != INVALID_SOCKET) {
            int id = -1;//pour donner un id au joueur
            for (int i = 0; i < MAX_PLAYER_NUMBER; i++)//<= pour parcourir une case de plus et si tout est full alors le player sera placé à la fin (id = size)
            {
                if (players[i] == nullptr) { id = i; break; }//si on trouve un trou, on met id à i et on sort de la boucle
            }
            if (id == -1) 
            { 
                cout << "Le serveur est plein..." << endl; //fermer la connexion en fermant le socket et pas en voyant id -1 ???
                NetworkEntity ne = { id, 100, 0, 0 };//envoie de l'entité avec l'id à -1 pour dire au client que le serveur est plein et fermer son programme
                Player p(tcpSocket, id, ne.hp, ne.xMap / 100, ne.yMap / 100);
                p.sendNETCP(ne);
                continue; 
            }
            NetworkEntity ne = { id, 0, 50, (1920 + 800) * 100, (1080 + 500) * 100, 0 };//structure pour communiquer dans le réseau
            Player* p = new Player(tcpSocket, id, ne.hp, ne.xMap / 100, ne.yMap / 100);
            p->sendNETCP(ne);//on envoie au joueur sa propre position
            cout << "PLAYERS SIZE: " << players.size() << endl;
            players[id] = p;
            NetworkEntity ne2;
            for (auto it = players.begin(); it != players.end(); ++it)//on envoie au nouveau joueur la position de tous ceux déjà connectés (la sienne comprise car on vient de l'ajouter)
            {
                if (it->second != p)
                {
                    it->second->setNE(ne2);
                    p->sendNETCP(ne2);
                }
            }
            send_NETCP(ne, p);//On envoie la position du nouveau joueur à tous ceux déjà connectés


            cout << "Connection succeed !" << endl << players.size() << " clients " << id << " connected !" << endl;
        }
        else std::cerr << "Connexion échoué -> erreur: " << WSAGetLastError() << std::endl;
        std::cout << "Waiting for new connection..." << std::endl;
    }
}

std::string timestampToTimeString(uint64_t timestamp) {
    time_t rawTime = static_cast<time_t>(timestamp);
    struct tm localTime;
    localtime_s(&localTime, &rawTime);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);

    return buffer;
}

void Server::listen_clientsTCP()
{
    fd_set readfds;//structure pour surveiller un ensemble de descripteurs de fichiers pour lire (ici les sockets)
    timeval timeout;
    char recvbuf[512];
    const int recvbuflen = 512;
    
    while (run)
    {
        FD_ZERO(&readfds);//reset l'ensemble &readfds
        for (auto it = players.begin(); it != players.end(); ++it)//inutile de lock, la partie qui supprime est après dans la même fonction donc pas de concurrence
        {
            //if (it->second->isSocketValid()) continue;
            if(it->second && it->second->isSocketValid()) FD_SET(*it->second->getTCPSocket(), &readfds);
        }
        // Vérifiez si le fd_set est vide
        if (readfds.fd_count == 0) {
            // Aucun socket à surveiller, attendez un peu avant de réessayer
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int iResult = select(0, &readfds, NULL, NULL, &timeout);//Attend que l'un des sockets dans readfds soit prêt pour la lecture (ou jusqu'à ce que le délai d'attente expire)
        if (iResult == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        for (auto it = players.begin(); it != players.end();) 
        {
            if (!it->second) continue;
            SOCKET* clientSocket = it->second->getTCPSocket();
            if (clientSocket && FD_ISSET(*clientSocket, &readfds)) {
                char buffer[recvbuflen];
                iResult = recv(*clientSocket, buffer, recvbuflen, 0);
                if (iResult > 0) {
                    it->second->recvBuffer.insert(it->second->recvBuffer.end(), buffer, buffer + iResult);

                    while (it->second->recvBuffer.size() >= sizeof(NetworkEntity)) {
                        NetworkEntity ne;
                        std::memcpy(&ne, it->second->recvBuffer.data(), sizeof(NetworkEntity));
                        ne.id        = ntohs(ne.id);
                        ne.hp        = ntohs(ne.hp);
                        ne.countDir  = ntohs(ne.countDir);
                        ne.xMap      = ntohl(ne.xMap);
                        ne.yMap      = ntohl(ne.yMap);
                        ne.timestamp = ntohll(ne.timestamp); 
                        uint64_t now = static_cast<uint64_t>(std::time(nullptr));//on update le timestamp à maintenant
                        //cout << timestampToTimeString(ne.timestamp) << " : " << timestampToTimeString(now) << endl;
                        //cout << ne.timestamp << " : " << now << endl;
                        if (ne.timestamp > (now - 5))//on traite les données reçu
                        {
                            // Process the received short data
                            it->second->update(ne);
                            //cout << "NE received: " << ne.id << " : " << ne.countDir << " : " << ne.xMap << " : " << ne.yMap << " : " << ne.timestamp << endl;
                            send_NETCP(ne, it->second);
                        }
                        else//on ne les traite pas et on renvoie la dernière position
                        {
                            it->second->setNE(ne);//met à jour la NE avec les data du joueur dernièrement enregistrées avant de les renvoyer
                            it->second->sendNETCP(ne);//modifie les données du timestamp pour les envoyer, on ne peut donc pas enregistrer le timestamp ensuite
                        }
                        it->second->recvBuffer.erase(it->second->recvBuffer.begin(), it->second->recvBuffer.begin() + sizeof(NetworkEntity));
                    }
                } else if (iResult == 0 || iResult == SOCKET_ERROR) {//UTILISER UN BOOL POUR BLOQUER LES ACTIONS QUAND LE SERVER DELETE ??
                    //mtx_sendNETCP.lock();
                    //isDeleting = true;// DOUBLE BOOL NECESSAIRE??!!!

                    NetworkEntity ne{ it->second->getID(), 0, 0, 0, -1 };
                    send_NETCP(ne, it->second);//on envoie le signalement de deconnexion du joueur avec un timestamp à -1 (= deconnexion)
                    if (it->second)
                    {
                        delete it->second;
                        it->second = nullptr;
                    }
                    it = players.erase(it);
                    //mtx_sendNETCP.unlock();
                    std::cout << "A client has been disconnected, " << players.size() << " left" << std::endl;
                    continue;
                }
            }
            ++it;
        }
    }
}

void Server::listen_clientsUDP()
{
    cout << "UDP LISTENING" << endl;

    fd_set readfds;
    timeval timeout;

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
            NetworkEntity ne;
            int clientAddrLen = sizeof(clientAddr);

            int bytesReceived = recvfrom(udpSocket, (char*)&ne, sizeof(ne), 0, (sockaddr*)&clientAddr, &clientAddrLen);
            if (bytesReceived <= 0) std::cerr << "Erreur lors de la reception des donnees." << WSAGetLastError() << std::endl;

            // Vérifiez si le message reçu est complet en comparant la taille des données reçues avec la taille attendue.
            if (bytesReceived != sizeof(ne)) {
                std::cerr << "Le message UDP reçu est incomplet." << std::endl;
                continue; // Si le message n'est pas complet, passez à l'itération suivante.
            }

            if (players[ne.id] != nullptr)
            {
                ne.id = htonl(ne.id);
                ne.xMap = htonl(ne.xMap);
                ne.yMap = htonl(ne.yMap);
                float xMap = (float)ne.xMap / 100;
                float yMap = (float)ne.yMap / 100;
                players[ne.id]->setAddr(clientAddr);
                players[ne.id]->setPos(xMap, yMap);
                sockaddr_in cli = *players[ne.id]->getPAddr();
                char clientIp[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(cli.sin_addr), clientIp, INET_ADDRSTRLEN);
                cout << players[ne.id]->getID() << " : " << players[ne.id]->getXMap() << " : " << players[ne.id]->getYMap() << " : " << clientIp << endl;
            }


            //ne.id = htonl(ne.id);
            //ne.xMap = htonl(ne.xMap);
            //ne.yMap = htonl(ne.yMap);
            //int bytesSent = sendto(udpSocket, (const char*)&ne, sizeof(ne), 0, (sockaddr*)clients[ne.id]->getPAddr(), clients[ne.id]->getAddrLen());
            //if (bytesSent == SOCKET_ERROR) {
            //    std::cerr << "Erreur lors de l'envoi des donnees -> " << WSAGetLastError() << std::endl;
            //}

        }
    }  
}

bool Server::recv_NEUDP(NetworkEntity& ne, sockaddr_in clientAddr)
{
    int clientAddrLen = sizeof(clientAddr);
    int bytesReceived = recvfrom(udpSocket, (char*)&ne, sizeof(ne), 0, (sockaddr*)&clientAddr, &clientAddrLen);
    if (bytesReceived <= 0) 
    {
        std::cerr << "Erreur lors de la reception des donnees." << WSAGetLastError() << std::endl; 
        return false;
    }

    ne.id = htonl(ne.id);
    ne.xMap = htonl(ne.xMap);
    ne.yMap = htonl(ne.yMap);

    return true;
}

void Server::send_NEUDP()
{
    NetworkEntity ne = { 0, 250, 250 };
    ne.id = htonl(ne.id);
    ne.xMap = htonl(ne.xMap * 100);
    ne.yMap = htonl(ne.yMap * 100);
    while (run)
    {
        for (auto it = players.begin(); it != players.end(); ++it)
        {
            if (it->second == nullptr) continue;
            for (auto it2 = players.begin(); it2 != players.end(); ++it2)
            {
                if (players[it->second->getID()]->getPAddr()->sin_family != AF_INET) continue; //vérifie si l'adresse est initialisée
                if (it2->second == nullptr) continue;
                ne.id = htonl(it2->second->getID());
                ne.xMap = htonl(it2->second->getXMap() * 100);
                ne.yMap = htonl(it2->second->getYMap() * 100);
                // Envoi des données sérialisées de chaque joueur connecté à it
                int bytesSent = sendto(udpSocket, (const char*)&ne, sizeof(ne), 0, (sockaddr*)players[it->second->getID()]->getPAddr(), players[it->second->getID()]->getAddrLen());
                if (bytesSent == SOCKET_ERROR) {
                    std::cerr << "Erreur lors de l'envoi des donnees -> " << WSAGetLastError() << std::endl;
                }
            }
        }
    }
}

void Server::send_NETCP(NetworkEntity ne, Player* p)
{
    //mtx_sendNETCP.lock();
    for (auto it = players.begin(); it != players.end(); ++it)
    {
        if (it->second == p) continue;//on n'envoie pas au joueur sa propre position
        it->second->sendNETCP(ne);
        //cout << "msg sent ofc" << endl;
    }
    //mtx_sendNETCP.unlock();
}

