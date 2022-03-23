#ifndef SERVER_HPP
# define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include "User.hpp"
#include "Command.hpp"
#include "Chanel.hpp"
#include "answers.h"

class User;
class Command;
class Chanel;

class Server
{
private:
	int										port;
	std::vector<struct pollfd>				userFDs;
	int										sockfd;
	sockaddr_in								sockaddr;
	const id_t								timeout;
	const std::string						password;
	std::vector<User*>						users;
	std::vector<Chanel*>					chanels;
	std::string								name;
public:
	Server(int port, char *pass);
	~Server();

	//server

	void									create_socket();
	void									bind_socket();
	void									listen_socket();
	void									grab_connection();
	void									process_messages();

	// cmd

	/*	Разбивает пришедшее сообщение на команды по \n и отправляет их на выполнение
		Параллельно выводит команды на экран (input - строка на ввод, cmd_init - пользователь отправивший сообщение) */
	void									cmd_handler(const char *input, User *cmd_init);

	/*	Перврящает строку в комманду в объект класса Command и запускает метод принятой команды
		(incmdput - строка с командой, cmd_init - пользователь отправивший сообщение)*/
	void									execute_command(char *cmd, User *cmd_init);

	int										cmd_pass(Command to_execute, User *cmd_init);
	int										cmd_user(Command to_execute, User *cmd_init);
	int										cmd_nick(Command to_execute, User *cmd_init);
	int										cmd_privmsg(Command to_execute, User *cmd_init, bool notice);
	int										cmd_away(Command to_execute, User *cmd_init);
	int										cmd_ping(Command to_execute, User *cmd_init);
	int										cmd_pong(Command to_execute, User *cmd_init);
	int										cmd_join(Command to_execute, User *cmd_init);
	void									cmd_quit(Command to_execute, User *cmd_init);
	void									cmd_online(User *cmd_init);
	void									cmd_ison(Command to_execute, User *cmd_init);
	void									send_motd(Command to_execute, User *cmd_init);
	void									send_response(Command to_execute, const std::string from, User *cmd_init, int responce);

	// utils
	/* Отправляет сообщение massage пользователю usr_ptr*/
	void									send_string_to_user(User *usr_ptr, std::string massage);

	/* Проверяет есть ли пользователь с FD new_user_fd*/
	bool									have_user(int new_user_fd);

	/* Возвращает указатель на пользователя с FD user_fd*/
	User									*find_user_by_fd(int user_fd);

	/* Возвращает указатель на пользователя с ником user_nick*/
	User									*find_user_by_nick(std::string user_nick);

	Chanel									*find_chanel_by_name(std::string chanel_name);
};

// void	SplitString(std::string s, std::vector<std::string> &v, char delimiter);
char	**ft_split(char const *s, char c);
void	print_word_by_letters(std::string word);
int		ft_length(const char *str);
int		ft_isalpha(int c);
int		ft_isdigit(int c);
int		ft_isspec(int c);
bool	nick_is_valid(std::string nick);
int		string_compare(std::string str1, std::string str2);
#endif