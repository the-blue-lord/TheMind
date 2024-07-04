#define SERVER_CPP

#include <iostream>
#include <SFML/Network.hpp>

#ifndef	THEMIND_HPP
#include "../themind-sfml.hpp"
#endif

int main(void) {
	int server_port = 32000;

    std::cout << "[SYSTEM] Initializing port " << server_port <<"...\n";

	sf::TcpListener listener;
	if(listener.listen(server_port) != sf::Socket::Done) {
		std::cout << "[ERROR] Server couldn't connect to port " << server_port << "\n";
		exit(1);
	}

	std::cout << "[SYSTEM] Server ready to receive on port " << server_port << "\n";

	TheMind Game(&listener);

	exit(0);
}
