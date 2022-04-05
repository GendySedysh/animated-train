#include "Channel.hpp"

Channel::Channel(std::string &name, User *creator, std::string &key)
{
	this->name = name;
	this->key = key;
	this->user_limit = 30;
	this->topic = "";
	this->flags = 0b00000000;
	this->operators.push_back(creator);
	this->users.push_back(creator);

	if (this->key.length() > 0)
		set_flag(CH_KEYSTATUS);
	set_flag(CH_NOMSGFROMOUT);
	set_flag(CH_LIMITED);
}

Channel::~Channel()
{
}

std::vector<std::string>	Channel::get_user_name_vec() {
	std::vector<std::string> user_names;

	for (size_t i = 0; i < users.size(); i++) {
		user_names.push_back(users[i]->get_nick());
	}
	return (user_names);
}

std::string	Channel::get_name() { return this->name; }
void		Channel::set_name(std::string new_name) { this->name = new_name; }

bool	Channel::is_operator(User *user) {
	for (size_t i = 0; i < operators.size(); i++) {
		if (user->get_nick().compare(operators[i]->get_nick()) == 0)
			return true;
	}
	return false;
}

bool	Channel::is_invited(User *user) {
	for (size_t i = 0; i < invited_users.size(); i++) {
		if (user->get_nick().compare(invited_users[i]->get_nick()) == 0)
			return true;
	}
	return false;
}

bool	Channel::is_in_channel(User *user) {
	for (size_t i = 0; i < users.size(); i++) {
		if (users[i]->get_nick().compare(user->get_nick()) == 0)
			return true;
	}
	return false;
}

int		Channel::add_user_to_channel(User *user, std::string &key) {
	std::string	to_send;

	if (this->is_limited() == true && ((this->user_limit - users.size()) <= 0))
		return ERR_CHANNELISFULL;
	if (this->is_keyed() == true && key != this->key) {
		return ERR_BADCHANNELKEY;
	} else if (this->is_invite_only() && !is_invited(user)) {
		return ERR_INVITEONLYCHAN;
	} else {
		if (is_in_channel(user)) {
			return ERR_USERONCHANNEL;
		} else {
			users.push_back(user);
			remove_invited(user);
			to_send = ":" + user->get_info_string() + " JOIN :" + name + "\n";
			send_string_to_channel(to_send);
		}
	}
	return 0;
}

int		Channel::delete_user_from_channel(User *user) {
	for (size_t i = 0; i < users.size(); i++) {
		if (user == users[i]){
			if (is_operator(users[i]) == true)
			{
				if (operators.size() == 1 && users.size() >= 2)
				{
					std::string to_send;

					to_send = ":" + operators[0]->get_info_string() + " MODE " + this->name + " +o" + " " + users[1]->get_nick() + "\n";
					send_string_to_channel(to_send);
					add_user_to_channel_operator(users[1]);
				}
			}
			users.erase(users.begin() + i);
		}
	}
	return 1;
}

int		Channel::add_user_to_channel_operator(User *user) {this->operators.push_back(user); return 0;}

int		Channel::delete_user_from_channel_operator(User *user){
	for (size_t i = 0; i < operators.size(); i++) {
		if (user == operators[i])
			operators.erase(operators.begin() + i);
	}
	return 1;
}

void	Channel::delete_offline_users() {
	std::string to_send;

	for (size_t i = 0; i < users.size(); i++)
	{
		if (users[i]->get_auth_status() == false)
		{
			delete_user_from_channel(users[i]);
			to_send = ":" + users[i]->get_info_string() + " PART " + this->get_name() + "\n";
			this->send_string_to_channel(to_send);
		}
	}

	for (size_t i = 0; i < operators.size(); i++) {
		if (operators[i]->get_auth_status() == false)
			delete_user_from_channel_operator(operators[i]);
	}
}

void	Channel::send_message_to_channel(std::string message, Server *server, bool notice, User *sender) {
	std::string header;

	for (size_t i = 0; i < users.size(); i++) {
		if (notice == false)
			header = ":" + sender->get_nick() + "!" + sender->get_username() + "@" + sender->get_address() + " PRIVMSG " + this->get_name() + " :";
		else
			header = ":" + sender->get_nick() + "!" + sender->get_username() + "@" + sender->get_address() + " NOTICE " + this->get_name() + " :";
		if (users[i] != sender && users[i]->get_auth_status() == true) {
			server->send_string_to_user(users[i], header);
			server->send_string_to_user(users[i], message);
		}
		header.clear();
	}
}

void	Channel::send_string_to_channel(std::string message) {
	for (size_t i = 0; i < users.size(); i++)
		send(users[i]->get_fd(), const_cast<char*>(message.c_str()), message.length(), 0);
}

void	Channel::set_flag(unsigned char flag)
{
	this->flags |= flag;
}

void	Channel::unset_flag(unsigned char flag)
{
	this->flags &= ~flag;
}


bool	Channel::is_private() const {
	return static_cast<bool>(this->flags & CH_PRIVATE);
}

bool	Channel::is_secret() const {
	return static_cast<bool>(this->flags & CH_SECRET);
}

bool	Channel::is_invite_only() const {
	return static_cast<bool>(this->flags & CH_INVITEONLY);
}

bool	Channel::is_topic_set_by_operator() const {
	return static_cast<bool>(this->flags & CH_TOPICSETOP);
}

bool	Channel::is_reachable_from_outside() const {
	return static_cast<bool>(this->flags & CH_NOMSGFROMOUT);
}

bool	Channel::is_moderated() const {
	return static_cast<bool>(this->flags & CH_MODERATED);
}

bool	Channel::is_limited() const {
	return static_cast<bool>(this->flags & CH_LIMITED);
}
bool	Channel::is_keyed() const {
	return static_cast<bool>(this->flags & CH_KEYSTATUS);
}

std::string	Channel::flag_status() {
	std::string	flags;
	std::string output;

	if (is_invite_only() == true)
		flags += 'i';
	if (is_topic_set_by_operator() == true)
		flags += 't';
	if (is_reachable_from_outside() == true)
		flags += 'n';
	if (is_keyed() == true)
		flags += 'k';
	if (is_limited() == true)
		flags += 'l';
	if (flags.length() > 0)
		output = '+' + flags;
	return output;
}

int		Channel::invite(User *sender, User *reciever) {
	if (this->is_invite_only() && !is_operator(sender)) {
		return (ERR_CHANOPRIVSNEEDED);
	} else {
		invited_users.push_back(reciever);
		return (RPL_INVITING);
	}
}

void	Channel::remove_invited(User *user) {
	if (is_invited(user)) {
		for (size_t i = 0; i < invited_users.size(); i++) {
			if (invited_users[i] == user) {
				invited_users.erase(invited_users.begin() + i);
				return ;
			}
		}
	}
}

int		Channel::set_topic(User *user, const std::string &topic)
{
	if (is_topic_set_by_operator() && !is_operator(user))
		return ERR_CHANOPRIVSNEEDED;
	this->topic = topic;
	return 0;
}

const std::string		&Channel::get_topic() const {
	return (this->topic);
}

void		Channel::set_key(std::string	new_key) { this->key = new_key; }
std::string	Channel::get_key() { return this->key; }

void		Channel::set_limit(int new_key) { this->user_limit = new_key; }
int			Channel::get_limit() { return this->user_limit; }