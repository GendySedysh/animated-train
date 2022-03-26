#ifndef CHANEL_HPP
# define CHANEL_HPP

#include "Server.hpp"

class Server;
class User;

class Channel
{
private:
	std::vector<User *>		operators;
	std::vector<User *>		users;
	std::string				name;
public:
	Channel(std::string name, User *creator);
	~Channel();

	std::string	get_name();
	// int							get_num_of_users();
	std::vector<std::string>	get_user_name_vec();
	bool	is_operator(User *user);
	bool	is_in_channel(User *user);
	int		add_user_to_channel(User *user);
	int		delete_user_from_channel(User *user);
	int		add_user_to_channel_operator(User *user);
	void	send_message_to_channel(std::string message, Server *server, bool notice, User *sender);
};

#endif