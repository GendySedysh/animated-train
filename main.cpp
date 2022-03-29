#include "Server.hpp"
#include <csignal>

bool	work = true;

void	sigHandler(int signum)
{
	(void)signum;
	work = false;
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
		exit(EXIT_FAILURE);
	}

	int port = atoi(argv[1]);

	if (port < 1024 || port > 49151)
	{
		std::cout << "Wrong port!" << std::endl;
		exit(EXIT_FAILURE);
	}

	Server		server(port, argv[2]);

	// Create a socket (IPv4, TCP)
	server.create_socket();

	// Listen to port on any address
	server.bind_socket();

	// Start listening.
	server.listen_socket();

	signal(SIGINT, sigHandler);

	while (work)
	{
		// Grab a connection from the queue
		server.grab_connection();

		server.process_messages();

		server.check_users();
	}
}
