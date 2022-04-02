#include "User.hpp" 

User::User(int new_fd): user_fd(new_fd), autorized(false), pass_ok(false), nick_ok(false), username_ok(false)
{
}

User::~User()
{
}

int				User::get_fd() { return this->user_fd; }
void			User::set_nick(std::string data) { this->nick = data; }
void			User::set_username(std::string data) { this->username = data; }
void			User::set_realname(std::string data) { this->realname = data; }
void			User::set_pass_status(bool status) { this->pass_ok = status; }
void			User::set_nick_status(bool status) { this->nick_ok = status;}
void			User::set_username_status(bool status) { this->username_ok = status;}
void			User::set_auth_status(bool status) { this->autorized = status;}
void			User::set_away_message(std::string message) { this->away_message = message; }
void			User::set_away_on(bool status) { this->away_on = status; }
void			User::set_address(std::string message) { this->address = message; }
bool			User::get_auth_status() { return this->autorized; }
bool			User::get_pass_ok() { return this->pass_ok; }
bool			User::get_nick_ok() { return this->nick_ok; }
bool			User::get_username_ok() { return this->username_ok; }
std::string		User::get_nick(void) { return this->nick; }
std::string		User::get_username(void) { return this->username; }
std::string		User::get_realname(void) { return this->realname; }
std::string		User::get_away_message() { return this->away_message; }
std::string		User::get_address() { return this->address; }
bool			User::get_away_on() { return this->away_on; }
bool			User::autorization_check() {
	if (this->pass_ok && this->nick_ok && this->username_ok)
		return true;
	return false;
}

void			User::send_message(const std::string &msg) const {
	if (msg.size() > 0)
		send(user_fd, msg.c_str(), msg.size(), 0);
}

std::string	User::get_info_string() const
{
	return std::string(nick + "!" + username + "@" + address);
}
