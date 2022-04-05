#ifndef CHANEL_HPP
# define CHANEL_HPP

#include "Server.hpp"
#include "answers.h"

class Server;
class User;

class Channel
{
private:
	std::vector<User *>		operators;
	std::vector<User *>		users;
	std::vector<User *>		invited_users;
	std::string				name;
	std::string				key;
	unsigned char			flags;
	int						user_limit;
	std::string				topic;
	

public:
	Channel(std::string &name, User *creator, std::string &key);
	~Channel();

	std::string	get_name();
	std::vector<std::string>	get_user_name_vec();
	bool	is_operator(User *user);
	bool	is_in_channel(User *user);
	int		add_user_to_channel(User *user, std::string &key);
	int		delete_user_from_channel(User *user);
	int		add_user_to_channel_operator(User *user);
	int		delete_user_from_channel_operator(User *user);
	void	delete_offline_users();
	void	send_message_to_channel(std::string message, Server *server, bool notice, User *sender);
	void	send_string_to_channel(std::string message);

	void		set_key(std::string	new_key);
	std::string	get_key();

	void		set_limit(int new_key);
	int			get_limit();

	// Flags methods
	void	set_flag(unsigned char flag);
	void	unset_flag(unsigned char flag);

	bool	is_private() const;
	bool	is_secret() const;
	bool	is_invite_only() const;
	bool	is_topic_set_by_operator() const;
	bool	is_reachable_from_outside() const;
	bool	is_moderated() const;
	bool	is_limited() const;
	bool	is_keyed() const;

	std::string	flag_status();

	int		invite(User *sender, User *reciever);
	bool	is_invited(User *user);
	void	remove_invited(User *user);
	const std::string	&get_topic() const;
	int					set_topic(User *user, const std::string &topic);
};

#endif