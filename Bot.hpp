#ifndef BOT_HPP
# define BOT_HPP

#include <curl/curl.h>
#include <cstring>
#include "Server.hpp"

class User;
class Command;
class Server;

class Bot
{
public:
	void	get_command(Command to_execute, User *cmd_init, Server *server);
	void	weather_data(std::string text, User *cmd_init, Server *server);
	// static std::string get_response(std::string ip, bool geo);
};

#endif