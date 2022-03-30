#include "Channel.hpp"

Channel::Channel(std::string &name, User *creator, std::string &key)
{
	this->name = name;
	this->key = key;
	this->operators.push_back(creator);
	this->users.push_back(creator);
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
	if (this->is_private() && key != this->key) {
		return ERR_BADCHANNELKEY;
	} else if (this->is_invite_only() && !is_invited(user)) {
		return ERR_INVITEONLYCHAN;
	} else {
		if (is_in_channel(user)) {
			return ERR_USERONCHANNEL;
		} else {
			users.push_back(user);
			remove_invited(user);
		}
	}
	return 0;
}

int		Channel::delete_user_from_channel(User *user) {
	for (size_t i = 0; i < users.size(); i++) {
		if (user == users[i])
			users.erase(users.begin() + i);
	}
	return 1;
}
int		Channel::add_user_to_channel_operator(User *user) {this->operators.push_back(user); return 0;}


void	Channel::send_message_to_channel(std::string message, Server *server, bool notice, User *sender) {
	std::string header;

	for (size_t i = 0; i < users.size(); i++) {
		if (notice == false)
			header = ":" + sender->get_nick() + "!" + sender->get_username() + "@" + sender->get_address() + " PRIVMSG " + this->get_name() + " :";
		else
			header = ":" + sender->get_nick() + "!" + sender->get_username() + "@" + sender->get_address() + " NOTICE " + this->get_name() + " :";
		if (is_in_channel(sender) == true && users[i] != sender && users[i]->get_auth_status() == true) {
			server->send_string_to_user(users[i], header);
			server->send_string_to_user(users[i], message);
		}
		header.clear();
	}
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
	return !(static_cast<bool>(this->flags & CH_NOMSGFROMOUT));
}

bool	Channel::is_moderated() const {
	return static_cast<bool>(this->flags & CH_MODERATED);
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