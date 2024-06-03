#include "Server.h"

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

    cout << "Socket TCP a l'ecoute" << endl;

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
    cout << "Socket UDP a l'ecoute" << endl;

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
                uti::NetworkEntity ne = { id, 100, 0, 0 };//envoie de l'entité avec l'id à -1 pour dire au client que le serveur est plein et fermer son programme
                Player p(tcpSocket, id, ne.hp, ne.xMap / 100, ne.yMap / 100);
                p.sendNETCP(ne);
                continue; 
            }
            uti::NetworkEntity ne = { 0, id, 0, 50, (1920 + 800) * 100, (1080 + 500) * 100, 0 };//structure pour communiquer dans le réseau
            Player* p = new Player(tcpSocket, id, ne.hp, ne.xMap / 100, ne.yMap / 100);
            p->sendNETCP(ne);//on envoie au joueur sa propre position
            p->sendNEFTCP({ uti::Header::NEF, (short)id, (short)((id % 2) + 1)});
            cout << "PLAYERS SIZE: " << players.size() << endl;
            p->setFaction(id % 2 + 1);
            players[id] = p;
            uti::NetworkEntity ne2;
            for (auto it = players.begin(); it != players.end(); ++it)//on envoie au nouveau joueur la position de tous ceux déjà connectés (la sienne comprise car on vient de l'ajouter)
            {
                if (it->second != p)
                {
                    it->second->setNE(ne2);
                    p->sendNETCP(ne2);
                    p->sendNEFTCP({ uti::Header::NEF, it->second->getID(), it->second->getFaction()});
                }
            }
            send_NETCP(ne, p);//On envoie la position du nouveau joueur à tous ceux déjà connectés
            send_NEFTCP({ uti::Header::NEF, (short)id, (short)((id % 2) + 1) }, p);


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
    fd_set readfds; // structure pour surveiller un ensemble de descripteurs de fichiers pour lire (ici les sockets)
    timeval timeout;
    const int recvbuflen = 512;

    while (run) {
        FD_ZERO(&readfds); // reset l'ensemble &readfds
        for (auto it = players.begin(); it != players.end(); ++it) { // inutile de lock, la partie qui supprime est après dans la même fonction donc pas de concurrence
            if (it->second && it->second->isSocketValid()) FD_SET(*it->second->getTCPSocket(), &readfds);
        }

        // Vérifiez si le fd_set est vide
        if (readfds.fd_count == 0) {
            // Aucun socket à surveiller, attendez un peu avant de réessayer
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int iResult = select(0, &readfds, NULL, NULL, &timeout); // Attend que l'un des sockets dans readfds soit prêt pour la lecture (ou jusqu'à ce que le délai d'attente expire)
        if (iResult == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        for (auto it = players.begin(); it != players.end();) {
            if (!it->second) {
                ++it;
                continue;
            }

            SOCKET* clientSocket = it->second->getTCPSocket();
            if (clientSocket && FD_ISSET(*clientSocket, &readfds)) {
                char buffer[recvbuflen];
                iResult = recv(*clientSocket, buffer, recvbuflen, 0);
                if (iResult > 0) {
                    it->second->recvBuffer.insert(it->second->recvBuffer.end(), buffer, buffer + iResult);

                    // Process received data
                    while (it->second->recvBuffer.size() >= sizeof(short)) {
                        // Check if we have enough data for the header
                        short header = 0;
                        std::memcpy(&header, it->second->recvBuffer.data(), sizeof(short));
                        header = ntohs(header);
                        cout << "Header: " << header << endl;

                        unsigned long dataSize = 0;
                        if (header == 0) {
                            dataSize = sizeof(uti::NetworkEntity);
                        }
                        else if (header == 1)
                        {
                            dataSize = sizeof(uti::NetworkEntitySpell);
                        }
                        else if (header == 2)
                        {
                            dataSize = sizeof(uti::NetworkEntitySpellEffect);
                        }

                        // Ajoutez d'autres conditions pour différents types de messages ici

                        if (it->second->recvBuffer.size() >= dataSize) {
                            // We have enough data to process the message
                            if (header == 0)
                            {
                                uti::NetworkEntity ne;
                                std::memcpy(&ne, it->second->recvBuffer.data(), sizeof(uti::NetworkEntity));
                                ne.header    = ntohs(ne.header);
                                ne.id        = ntohs(ne.id);
                                ne.hp        = ntohs(ne.hp);
                                ne.countDir  = ntohs(ne.countDir);
                                ne.xMap      = ntohl(ne.xMap);
                                ne.yMap      = ntohl(ne.yMap);
                                ne.timestamp = ntohll(ne.timestamp);
                                uint64_t now = static_cast<uint64_t>(std::time(nullptr));
                                if (ne.timestamp > (now - 5)) {
                                    // Process the received short data
                                    it->second->update(ne);
                                    send_NETCP(ne, it->second);
                                }
                                else {
                                    it->second->setNE(ne);
                                    it->second->sendNETCP(ne);
                                }
                            }
                            else if (header == 1)
                            {
                                uti::NetworkEntitySpell nes;
                                std::memcpy(&nes, it->second->recvBuffer.data(), sizeof(uti::NetworkEntitySpell));
                                nes.header  = ntohs(nes.header);
                                nes.id      = ntohs(nes.id);
                                nes.spellID = ntohs(nes.spellID);

                                send_NESTCP(nes, it->second);

                                cout << "NES RECEIVED: " << nes.header << " : " << nes.id << " : " << nes.spellID << endl;
                            }
                            else if (header == 2)
                            {
                                uti::NetworkEntitySpellEffect nese;
                                std::memcpy(&nese, it->second->recvBuffer.data(), sizeof(uti::NetworkEntitySpell));
                                nese.header  = ntohs(nese.header);
                                nese.id      = ntohs(nese.id);
                                nese.spellID = ntohs(nese.spellID);

                                players[nese.id]->dealDmg(20);
                                send_NETCP(players[nese.id]->getNE());

                                //send_NESETCP(nese, it->second);

                                cout << "NESE RECEIVED: " << nese.header << " : " << nese.id << " : " << nese.spellID << endl;
                            }
                            it->second->recvBuffer.erase(it->second->recvBuffer.begin(), it->second->recvBuffer.begin() + dataSize);
                        }
                        else {
                            break; // We don't have enough data yet
                        }
                    }
                }
                else if (iResult == 0 || iResult == SOCKET_ERROR) {
                    cout << "ERROR: " << WSAGetLastError() << " : " << iResult << endl;
                    uti::NetworkEntity ne{ it->second->getID(), 0, 0, 0, -1 };
                    send_NETCP(ne, it->second);
                    if (it->second) {
                        delete it->second;
                        it->second = nullptr;
                    }
                    it = players.erase(it);
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
            uti::NetworkEntity ne;
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
                //ne.spell = htons(ne.spell);
                float xMap = (float)ne.xMap / 100;
                float yMap = (float)ne.yMap / 100;
                players[ne.id]->setAddr(clientAddr);
                players[ne.id]->setPos(xMap, yMap);
                //players[ne.id]->spell = ne.spell;
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

bool Server::recv_NEUDP(uti::NetworkEntity& ne, sockaddr_in clientAddr)
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
    uti::NetworkEntity ne = { 0, 250, 250 };
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

void Server::send_NETCP(uti::NetworkEntity ne, Player* p)
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

void Server::send_NETCP(uti::NetworkEntity ne)
{
    //mtx_sendNETCP.lock();
    for (auto it = players.begin(); it != players.end(); ++it)
    {
        it->second->sendNETCP(ne);
        //cout << "msg sent ofc" << endl;
    }
    //mtx_sendNETCP.unlock();
}

void Server::send_NESTCP(uti::NetworkEntitySpell nes, Player* p)
{
    //mtx_sendNETCP.lock();
    for (auto it = players.begin(); it != players.end(); ++it)
    {
        if (it->second == p) continue;//on n'envoie pas au joueur sa propre position
        it->second->sendNESTCP(nes);
        //cout << "msg sent ofc" << endl;
    }
    //mtx_sendNETCP.unlock();
}

void Server::send_NESETCP(uti::NetworkEntitySpellEffect nese, Player* p)
{
    //mtx_sendNETCP.lock();
    for (auto it = players.begin(); it != players.end(); ++it)
    {
        if (it->second == p) continue;//on n'envoie pas au joueur sa propre position
        it->second->sendNESETCP(nese);
        //cout << "msg sent ofc" << endl;
    }
    //mtx_sendNETCP.unlock();
}

void Server::send_NEFTCP(uti::NetworkEntityFaction nef, Player* p)
{
    for (auto it = players.begin(); it != players.end(); ++it)
    {
        if (it->second == p) continue;//on n'envoie pas au joueur sa propre position
        it->second->sendNEFTCP(nef);
        //cout << "msg sent ofc" << endl;
    }
}

