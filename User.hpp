#ifndef USER_HPP
# define USER_HPP

#include "Server.hpp"

class User
{
private:
	std::string		nick;
	std::string		username;
	std::string		realname;
	std::string		away_massage;
	std::string		address;
	int				user_fd;
	bool			away_on;
	bool			autorized;
	bool			pass_ok;
	bool			nick_ok;
	bool			username_ok;
public:
	User(int new_fd);
	~User();

	bool			autorization_check();
	void			set_nick(std::string data);
	void			set_username(std::string data);
	void			set_realname(std::string data);
	void			set_pass_status(bool status);
	void			set_nick_status(bool status);
	void			set_username_status(bool status);
	void			set_auth_status(bool status);
	void			set_away_massage(std::string massage);
	void			set_away_on(bool status);
	void			set_address(std::string messege);
	int				get_fd();
	bool			get_auth_status();
	bool			get_pass_ok();
	bool			get_nick_ok();
	bool			get_username_ok();
	std::string		get_nick(void);
	std::string		get_username(void);
	std::string		get_realname(void);
	std::string		get_away_massage();
	std::string		get_address();
	bool			get_away_on();
};


#endif