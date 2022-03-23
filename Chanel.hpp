#ifndef CHANEL_HPP
# define CHANEL_HPP

#include "Server.hpp"

class Server;
class User;

class Chanel
{
private:
	std::vector<User *>		operators;
	std::vector<User *>		users;
	std::string				name;
public:
	Chanel(std::string name, User *creator);
	~Chanel();

	std::string	get_name();
	bool	is_operator(User *user);
	bool	is_in_channel(User *user);
	int		add_user_to_channel(User *user);
	int		delete_user_from_channel(User *user);
	int		add_user_to_channel_operator(User *user);
	void	send_messege_to_chanel(std::string messege, Server *server, bool notice, User *sender);
};

#endif