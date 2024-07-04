/*
*   Exit codes:
*       701 - Too many connection attempts tried by the client, the server searched is unreachable.
*       702 - Failed to receive a packet of data with the cards for the user
*       703 - Failed to receive a packet of data with the first card shown beetween the players
*       704 - Faiiled to retrive a card sent by a player
*/

#define THEMIND_HPP

#include <iostream>
#include <cstdlib>
#include <string>
#include <future>
#include <ctime>
#include <SFML/Network.hpp>

int getNewCardOnTable();

class ServerPlayer {
    public:
        std::string name;
        sf::TcpSocket client;

        int cardsInHand[12];
        
        ServerPlayer(sf::TcpListener *listener_ptr) {
            resetCardsInHand();

            std::cout << "[GAME] Waiting for a player to connect on port " << listener_ptr->getLocalPort() << "...\n";
            
            bool clientIsConnected;
            do {
                clientIsConnected = true;
                if(listener_ptr->accept(client) != sf::Socket::Done) {
                    std::cout << "[ERROR] There was an error while connecting to the client, retrying the connection...\n";
                    clientIsConnected = false;
                }
            } while(!clientIsConnected);

            std::cout << "[PLAYER] Player " << client.getRemoteAddress() << " has connected, retriving the username...\n";
            
		    sf::Packet name_packet;
            if(client.receive(name_packet) != sf::Socket::Done) {
                std::cout << "[WARN] Failed to retrive the name, using ip adress (" << client.getRemoteAddress() << ")\n";
                name.assign("UknownUser");
            }
            else {
                name_packet >> name;
            }

            std::cout << "[PLAYER] " << name << " (" << client.getRemoteAddress() << ") has successfully connected to the server\n";
        }

        void resetCardsInHand() {
            for(int i = 0; i < 12; i++) cardsInHand[i] = 0;
            return;
        }

        void assignCards(int cardsToGive[12], int level) {
            sf::Packet NewCardsPacket;
            NewCardsPacket << level;

            resetCardsInHand();
            for(int i = 0; i < level; i++) {
                cardsInHand[i] = cardsToGive[i];
                NewCardsPacket << cardsToGive[i];
            }

        }

        void giveCard(int card) {
            int tempIntForSwitching = 0;
            for(int i = 0; i < 12; i++) {
                if(cardsInHand[i] == 0) {
                    cardsInHand[i] = card;
                    break;
                }
            }
        }

        void sendCardsToClient(int level) {
            sf::Packet NewPlayerCards;
            NewPlayerCards << level;
            for(int i = 0; i < level; i++) NewPlayerCards << cardsInHand[i];
            client.send(NewPlayerCards);
        }

        void sortCards() {
            bool cardsSorted = false;
            for(int i = 0; i < 12; i++) if(cardsInHand[i] == 0) cardsInHand[i] = 300;
            while(!cardsSorted) {
                cardsSorted = true;
                for(int i = 0; i < 11; i++) {
                    if(cardsInHand[i] > cardsInHand[i+1]) {
                        int tmp = cardsInHand[i];
                        cardsInHand[i] = cardsInHand[i+1];
                        cardsInHand[i+1] = tmp;

                        cardsSorted = false;
                    }
                }
            }
        }
};

class ClientPlayer {
    public:
        std::string name;
        sf::TcpSocket client;

        int cardsInHand[12];
        int cardsOnTable[24];

        int level = 0;
        bool gameIsFinished = false;

        ClientPlayer(sf::IpAddress serverAdress, int serverPort) {
            resetCardsInHand();
            resetCardsOnTable();

            std::cout << "Choose your username (without spaces)> ";
            std::cin >> name;

            std::cout << "[GAME] Connecting to server " << serverAdress << " on port " << serverPort << "...\n";
            
            int connectionAttempts = 0;
            bool clientIsConnected;
            do {
                clientIsConnected = true;
                if(client.connect(serverAdress, serverPort) != sf::Socket::Done) {
                    std::cout << "[ERROR] There was an error while connecting to the server, reattemping the connection...\n";
                    clientIsConnected = false;
                }
                connectionAttempts++;
            } while(!clientIsConnected && connectionAttempts <= 5);

            if(connectionAttempts > 5) {
                std::cout << "[FATAL ERROR] Too many connection attempts, exiting with code 701\n";
                exit(701);
            }

            std::cout << "[GAME] Established connection with the server, sending the username...\n";

            sf::Packet NamePacket;
            NamePacket << name;
            if(client.send(NamePacket) != sf::Socket::Done) {
                std::cout << "[WARN] Failed to send the username, you will be seen as \"UnknownUser\"\n";
            }

            std::cout << "[GAME] You successfully connected to the server\n";

            while(1) {
                resetCardsOnTable();
                sf::Packet NewCardsPacket;
                if(client.receive(NewCardsPacket) != sf::Socket::Done) {
                    std::cout << "[FATAL ERROR] Failed to receive the cards, exiting with code 702\n";
                    exit(702);
                }
                NewCardsPacket >> level;
                if(level == 13) break;
                resetCardsInHand();
                std::cout << "\n\n\n------- LEVEL " << level << " --------------\n";
                std::cout << "[LEVEL] Entered level " << level << "\n";

                bool roundIsFinished = false;
                while(!roundIsFinished) {
                    std::cout << "\n[DECK] Your deck is:";
                    for(int i = 0; i < level; i++) {
                        NewCardsPacket >> cardsInHand[i];
                        if(cardsInHand[i] != 0) std::cout << " " << cardsInHand[i];
                    }
                    std::cout << "\n";

                    char ans = 'n';
                    if(cardsInHand[0] != 0) std::cout << "[DECK] Show " << cardsInHand[0] << " (y/n)> ";
                    else std::cout << "[DECK] Notify your empty deck (any character)> ";
                    std::cin >> ans;
                    sf::Packet ShowCardPacket;
                    if(cardsInHand[0] == 0) ShowCardPacket << 101;
                    else if(ans == 'y') ShowCardPacket << cardsInHand[0];
                    else ShowCardPacket << 0;

                    client.send(ShowCardPacket);

                    sf::Packet NewCardPacket;
                    if(client.receive(NewCardPacket) != sf::Socket::Done) {
                        std::cout << "[FATAL ERROR] Failed to receive the first card shown, exiting with code 703\n";
                        exit(703);
                    }
                    int newCard = 0;
                    NewCardPacket >> newCard;
                    int tempCodeStorer = 0;
                    std::cout << newCard << " - " << tempCodeStorer << "\n";
                    if(newCard == 201 || newCard == 202) {
                        tempCodeStorer = newCard;
                        NewCardPacket >> newCard;
                    }
                    std::cout << newCard << " - " << tempCodeStorer << "\n";

                    for(int i = 0; i < 24; i++) {
                        if(cardsOnTable[i] == 0) {
                            if(1 <= newCard && newCard <= 100) cardsOnTable[i] = newCard;
                            break;
                        }
                    }
                    std::cout << newCard << " - " << tempCodeStorer << "\n";

                    if(tempCodeStorer == 201 || tempCodeStorer == 202) newCard = tempCodeStorer;
                    std::cout << newCard << " - " << tempCodeStorer << "\n";
                    
                    if(newCard == cardsInHand[0]) {
                        for(int i = 0; i < 11; i++) cardsInHand[i] = cardsInHand[i+1];
                        cardsInHand[11] = 0;
                    }

                    if(newCard == 201) {
                        std::cout << "[GAME] The cards on the table are:";
                        for(int i = 0; cardsOnTable[i] != 0; i++) std::cout << " " << cardsOnTable[i];
                        std::cout << "\n[LEVEL] You remained the only player with cards. Level completed!\n";
                        roundIsFinished = true;
                        break;
                    }
                    if(newCard == 202) {
                        std::cout << "[GAME] The cards on the table are:";
                        for(int i = 0; cardsOnTable[i] != 0; i++) std::cout << " " << cardsOnTable[i];
                        std::cout << "\n[LEVEL] You remained without cards. Level completed!\n";
                        roundIsFinished = true;
                        break;
                    }

                    std::cout << "[GAME] The cards on the table are:";
                    for(int i = 0; cardsOnTable[i] != 0; i++) std::cout << " " << cardsOnTable[i];
                    std::cout << "\n";

                    if(newCard == 204) {
                        int wrongCard = 204;
                        NewCardPacket >> wrongCard;
                        std::cout << "[GAME] The card " << wrongCard << " was shown too late";
                        if(level == 1) std::cout << ", restarting the level";
                        else std::cout << ", returning back to previous level";
                        std::cout << "\n";
                        break;
                    }
                }
            }
            std::cout << "\n\n\n------- THE END -------------\n";
            std::cout << "[GAME] Congrats, you beated me! Hope to see you soon ;)\n";
            std::cout << "Ctrl + C (Cmd + C) to exit\n";
            while(1);
        }

        void resetCardsInHand() {
            for(int i = 0; i < 12; i++) cardsInHand[i] = 0;
        }

        void resetCardsOnTable() {
            for(int i = 0; i < 24; i++) cardsOnTable[i] = 0;
        }
};

class TheMind {
    public:
        int deck[100];
        int deckLength = 0;
        ServerPlayer *player[2];

        int cardsAlreadyUsed[24];

        int cardsOnTable[24];

        TheMind(sf::TcpListener *listener_ptr) {
            std::cout << "[GAME] Starting the game on port " << listener_ptr->getLocalPort() << "...\n";

            for(int i = 0; i < 100; i++) deck[i] = i+1; deckLength = 100;
            for(int i = 0; i < 24; i++) cardsOnTable[i] = 0;
            for(int i = 0; i < 2; i++) player[i] = new ServerPlayer(listener_ptr);
            srand(time(NULL));
            for(int i = 0; i < 12; i++) startLevel(i+1);
            std::cout << "[GAME] Game successfully completed";
        }

        void startLevel(int level) {
            player[0]->resetCardsInHand();
            player[1]->resetCardsInHand();

            for(int i = 0; i < 24; i++) {
                cardsAlreadyUsed[i] = 0;
                cardsOnTable[i] = 0;
            }
            std::cout << "[LEVEL] Entered level " << level << "\n";

            for(int i = 0; i < 2*level; i++) {
                if(i == 0) std::cout << "[LEVEL] Getting cards for " << player[0]->name << ":";
                else if(i == level) std::cout << "\n[LEVEL] Getting cards for " << player[1]->name << ":";

                int newCard = getRandomCardFromDeck();
                for(int j = 0; j < 24; j++) {
                    if(cardsAlreadyUsed[j] == 0) {
                        cardsAlreadyUsed[j] = newCard;
                        break;
                    }
                }
                player[i/level]->giveCard(newCard);
                std::cout << " " << newCard;
            }
            std::cout << "\n";

            player[0]->sortCards();
            player[1]->sortCards();

            player[0]->sendCardsToClient(level);
            player[1]->sendCardsToClient(level);

            sf::SocketSelector PlayersListener;
            PlayersListener.add(player[0]->client);
            PlayersListener.add(player[1]->client);
            bool roundIsFinished = false;
            bool levelCompleted = true;
            while(!roundIsFinished) {
                PlayersListener.wait();

                sf::Packet FirstCardPacket;
                sf::Packet SecondCardPacket;
                int firstPlayerIndex = -1, secondPlayerIndex = -1;
                if(PlayersListener.isReady(player[0]->client)) {
                    firstPlayerIndex = 0;
                    secondPlayerIndex = 1;
                }
                else if(PlayersListener.isReady(player[1]->client)) {
                    firstPlayerIndex = 1;
                    secondPlayerIndex = 0;
                }

                if(firstPlayerIndex == -1) {
                    std::cout << "\n[FATAL ERROR] Unable to retrive the card, exiting with code 704\n";
                    exit(704);
                }

                if(player[firstPlayerIndex]->client.receive(FirstCardPacket) != sf::Socket::Done) {
                    std::cout << "\n[FATAL ERROR] Unable to retrive the card, exiting with code 704\n";
                    exit(704);
                }
                if(player[secondPlayerIndex]->client.receive(SecondCardPacket) != sf::Socket::Done) {
                    std::cout << "\n[FATAL ERROR] Unable to retrive the card, exiting with code 704\n";
                    exit(704);
                }

                int firstCard = 0, secondCard = 0;

                FirstCardPacket >> firstCard;
                SecondCardPacket >> secondCard;

                std::cout << "[CARD] " << player[firstPlayerIndex]->name << " --> " << firstCard << "\n";
                std::cout << "[CARD] " << player[secondPlayerIndex]->name << " --> " << secondCard << "\n";

                sf::Packet FirstAnswerPacket;
                sf::Packet SecondAnswerPacket;

                int answer = 0;

                if(firstCard == 101) {
                    if(secondCard == 0) {
                        FirstAnswerPacket << 203;
                        SecondAnswerPacket << 203;
                        std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 203 << "\n";
                        std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 203 << "\n";
                    }
                    else {
                        int err = 0;
                        if(secondCard <= 100) err = addCardToTable(secondCard);
                        if(!err) {
                            FirstAnswerPacket << 202 << secondCard;
                            SecondAnswerPacket << 201 << secondCard;
                            std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 202 << " <-- " << secondCard << "\n";
                            std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 201 << " <-- " << secondCard << "\n";
                        }
                        else {
                            FirstAnswerPacket << 204 << secondCard;
                            SecondAnswerPacket << 204 << secondCard;
                            std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 204 << " <-- " << secondCard << "\n";
                            std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 204 << " <-- " << secondCard << "\n";
                            levelCompleted = false;
                        }

                        roundIsFinished = true;
                    }
                }
                else if(secondCard == 101) {
                    if(firstCard == 0) {
                        FirstAnswerPacket << 203;
                        SecondAnswerPacket << 203;
                        std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 203 << "\n";
                        std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 203 << "\n";
                    }
                    else {
                        int err = 0;
                        if(firstCard <= 100) err = addCardToTable(firstCard);
                        if(!err) {
                            FirstAnswerPacket << 201 << firstCard;
                            SecondAnswerPacket << 202 << firstCard;
                            std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 201 << " <-- " << firstCard << "\n";
                            std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 202 << " <-- " << firstCard << "\n";
                        }
                        else {
                            FirstAnswerPacket << 204 << firstCard;
                            SecondAnswerPacket << 204 << firstCard;
                            std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 204 << " <-- " << firstCard << "\n";
                            std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 204 << " <-- " << firstCard << "\n";
                            levelCompleted = false;
                        }

                        roundIsFinished = true;
                    }
                }
                else if(firstCard != 0) {
                    int err = addCardToTable(firstCard);
                    if(!err) {
                        FirstAnswerPacket << firstCard;
                        SecondAnswerPacket << firstCard;
                        std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << firstCard << "\n";
                        std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << firstCard << "\n";
                    }
                    else {
                        FirstAnswerPacket <<  204 << firstCard;
                        SecondAnswerPacket << 204 << firstCard;
                        std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 204 << " <-- " << firstCard << "\n";
                        std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 204 << " <-- " << firstCard << "\n";
                        levelCompleted = false;
                        roundIsFinished = true;
                    }
                }
                else if(secondCard != 0) {
                    int err = addCardToTable(secondCard);
                    if(!err) {
                        FirstAnswerPacket << secondCard;
                        SecondAnswerPacket << secondCard;
                        std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << secondCard << "\n";
                        std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << secondCard << "\n";
                    }
                    else {
                        FirstAnswerPacket <<  204 << secondCard;
                        SecondAnswerPacket << 204 << secondCard;
                        std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 204 << " <-- " << secondCard << "\n";
                        std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 204 << " <-- " << secondCard << "\n";
                        levelCompleted = false;
                        roundIsFinished = true;
                    }
                }
                else {
                    FirstAnswerPacket << 0;
                    SecondAnswerPacket << 0;
                    std::cout << "[CARD] " << player[firstPlayerIndex]->name << " <-- " << 0 << "\n";
                    std::cout << "[CARD] " << player[secondPlayerIndex]->name << " <-- " << 0 << "\n";
                }

                std::cout << "[TABLE]";
                for(int i = 0; cardsOnTable[i] != 0; i++) std::cout << " " << cardsOnTable[i];
                std::cout << "\n";

                player[firstPlayerIndex]->client.send(FirstAnswerPacket);
                player[secondPlayerIndex]->client.send(SecondAnswerPacket);


            }

            std::cout << "\n";

            if(!levelCompleted) {
                if(level == 1) startLevel(1);
                else {
                    startLevel(level-1);
                    startLevel(level);
                }
            }
        }

        int addCardToTable(int cardToAdd) {
            for(int i = 0; i < 24; i++) {
                if(cardsOnTable[i] == 0) {
                    cardsOnTable[i] = cardToAdd;
                    if(i != 0 && cardsOnTable[i-1] > cardsOnTable[i]) return 1;
                    break;
                }
            }
            return 0;
        }

        int getRandomCardFromDeck() {
            if(deckLength == 0) {
                for(int i = 0, possibleIndex = 0; i < 100; i++) {
                    if(i == cardsAlreadyUsed[possibleIndex]) {
                        possibleIndex++;
                        continue;
                    }
                    deck[deckLength] = i;
                    deckLength++;
                }
            }
            int newCardIndex = rand()%deckLength;
            int newCard = deck[newCardIndex];
            for(int i = newCardIndex+1; i < deckLength; i++) deck[i-1] = deck[i];
            deckLength--;
            deck[deckLength] = 0;
            return newCard;
        }
};