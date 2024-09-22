#include <iostream>
#include <SFML/Network.hpp>

#ifndef	THEMIND_HPP
#include "../themind-sfml.hpp"
#endif

int main(void) {
    sf::IpAddress serverAdress;
    std::cout << "Insert the server ip adress> ";
    std::cin >> serverAdress;
    int serverPort = 32000;
    std::cout << "Insert the server port> ";
    std::cin >> serverPort;

    ClientPlayer player(serverAdress, serverPort);

    exit(0);
}