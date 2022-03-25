#include "Chanel.hpp"

Chanel::Chanel(std::string name, User *creator)
{
	this->name = name;
	this->operators.push_back(creator);
	this->users.push_back(creator);
}

Chanel::~Chanel()
{
}

std::vector<std::string>	Chanel::get_user_name_vec() {
	std::vector<std::string> user_names;

	for (size_t i = 0; i < users.size(); i++) {
		user_names.push_back(users[i]->get_nick());
	}
	return (user_names);
}

std::string	Chanel::get_name() {return this->name; }

bool	Chanel::is_operator(User *user) {
	for (size_t i = 0; i < operators.size(); i++) {
		if (user->get_nick().compare(operators[i]->get_nick()) == 0)
			return true;
	}
	return false;
}

bool	Chanel::is_in_channel(User *user) {
	for (size_t i = 0; i < users.size(); i++) {
		if (users[i]->get_nick().compare(user->get_nick()) == 0)
			return true;
	}
	return false;
}

int		Chanel::add_user_to_channel(User *user) {this->users.push_back(user); return 0;}
int		Chanel::delete_user_from_channel(User *user) {
	for (size_t i = 0; i < users.size(); i++) {
		if (user == users[i])
			users.erase(users.begin() + i);
	}
	return 1;
}
int		Chanel::add_user_to_channel_operator(User *user) {this->operators.push_back(user); return 0;}
void	Chanel::send_messege_to_chanel(std::string messege, Server *server, bool notice, User *sender) {
	std::string header;

	for (size_t i = 0; i < users.size(); i++) {
		if (notice == false)
			header = ":" + sender->get_nick() + "!" + sender->get_username() + "@" + sender->get_address() + " PRIVMSG " + this->get_name() + " :";
		else
			header = ":" + sender->get_nick() + "!" + sender->get_username() + "@" + sender->get_address() + " NOTICE " + this->get_name() + " :";
		if (is_in_channel(sender) == true && users[i] != sender) {
			server->send_string_to_user(users[i], header);
			server->send_string_to_user(users[i], messege);
		}
		header.clear();
	}
}