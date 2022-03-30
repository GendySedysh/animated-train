#ifndef CHANEL_HPP
# define CHANEL_HPP

#include "Server.hpp"
#include "answers.h"

class Server;
class User;

#define CH_PRIVATE 0b00000001
#define CH_SECRET 0b00000010
#define CH_INVITEONLY 0b00000100
#define CH_TOPICSETOP 0b00001000
#define CH_NOMSGFROMOUT 0b00010000
#define CH_MODERATED 0b00100000

class Channel
{
private:
	std::vector<User *>		operators;
	std::vector<User *>		users;
	std::vector<User *>		invited_users;
	std::string				name;
	std::string				key;
	unsigned char			flags;
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
	void	send_message_to_channel(std::string message, Server *server, bool notice, User *sender);

	// Flags methods
	void	set_flag(unsigned char flag);
	void	unset_flag(unsigned char flag);

	bool	is_private() const;
	bool	is_secret() const;
	bool	is_invite_only() const;
	bool	is_topic_set_by_operator() const;
	bool	is_reachable_from_outside() const;
	bool	is_moderated() const;

	int		invite(User *sender, User *reciever);
	bool	is_invited(User *user);
	void	remove_invited(User *user);
	const std::string	&get_topic() const;
	int					set_topic(User *user, const std::string &topic);
};

#endif