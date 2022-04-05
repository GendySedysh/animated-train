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
		exit(0);
	}

	int port = atoi(argv[1]);

	if (port < 1024 || port > 49151)
	{
		std::cout << "Wrong port!" << std::endl;
		exit(0);
	}

	Server		server(port, argv[2]);

	server.create_socket();
	server.bind_socket();
	server.listen_socket();

	signal(SIGINT, sigHandler);

	while (work)
	{
		server.grab_connection();

		server.process_messages();

		server.check_users();

		server.check_channels();
	}
}
