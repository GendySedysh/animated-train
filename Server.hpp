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
#include "Channel.hpp"
#include "answers.h"
#include "Bot.hpp"

#define CH_PRIVATE		0b00000001
#define CH_SECRET		0b00000010
#define CH_INVITEONLY	0b00000100
#define CH_TOPICSETOP	0b00001000
#define CH_NOMSGFROMOUT 0b00010000
#define CH_MODERATED	0b00100000
#define CH_LIMITED		0b01000000
#define CH_KEYSTATUS	0b10000000
#define CH_OPERATOR		0b00000011

class User;
class Command;
class Channel;
class Server;
class Bot;

typedef  int (Server::*MappedCMD) (Command, User *); // using???

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
	std::vector<Channel*>					channels;
	std::string								name;
	Bot										*weather_bot;
	
	std::map<std::string, MappedCMD>		commands;
	std::map<char, unsigned int>				flags;
public:
	Server(int port, char *pass);
	~Server();

	//server

	void									create_socket();
	void									bind_socket();
	void									listen_socket();
	void									grab_connection();
	void									check_users();
	void									check_channels();
	int										process_messages();

	// cmd

	/*	Разбивает пришедшее сообщение на команды по \n и отправляет их на выполнение
		Параллельно выводит команды на экран (input - строка на ввод, cmd_init - пользователь отправивший сообщение) */
	void									cmd_handler(std::string input, User *cmd_init);

	/*	Перврящает строку в комманду в объект класса Command и запускает метод принятой команды
		(incmdput - строка с командой, cmd_init - пользователь отправивший сообщение)*/
	void									execute_command(std::string cmd, User *cmd_init);

	int										cmd_pass(Command to_execute, User *cmd_init);
	int										cmd_user(Command to_execute, User *cmd_init);
	int										cmd_nick(Command to_execute, User *cmd_init);
	int										cmd_privmsg(Command to_execute, User *cmd_init);
	int										cmd_away(Command to_execute, User *cmd_init);
	int										cmd_ping(Command to_execute, User *cmd_init);
	int										cmd_pong(Command to_execute, User *cmd_init);
	int										cmd_join(Command to_execute, User *cmd_init);
	int										cmd_kick(Command to_execute, User *cmd_init);
	int										cmd_part(Command to_execute, User *cmd_init);
	int										cmd_quit(Command to_execute, User *cmd_init);
	int										cmd_online(Command to_execute, User *cmd_init);
	int										cmd_ison(Command to_execute, User *cmd_init);
	int										cmd_topic(Command to_execute, User *cmd_init);
	int										cmd_invite(Command to_execute, User *cmd_init);
	int										cmd_mode(Command to_execute, User *cmd_init);
	void									send_motd(Command to_execute, User *cmd_init);
	int										send_response(const std::string from, User *cmd_init, int response,
														std::string arg1, std::string arg2, std::string arg3, std::string arg4);

	// utils
	/* Отправляет сообщение massage пользователю usr_ptr*/
	void									send_string_to_user(User *usr_ptr, std::string massage);

	/* Проверяет есть ли пользователь с FD new_user_fd*/
	bool									have_user(int new_user_fd);

	/* Возвращает указатель на пользователя с FD user_fd*/
	User									*find_user_by_fd(int user_fd);

	/* Возвращает указатель на пользователя с ником user_nick*/
	User									*find_user_by_nick(std::string user_nick);

	Channel									*find_channel_by_name(std::string chanel_name);
};

void	print_word_by_letters(std::string word);
int		ft_isalpha(int c);
int		ft_isdigit(int c);
int		ft_isspec(int c);
bool	nick_is_valid(std::string nick);
bool	channel_name_is_valid(std::string channel_name);
void	tokenize(std::string &str, const char delim, std::vector<std::string> &out);
#endif