#include "Server.hpp"

Server::Server(int port, char *pass): port(port), timeout(1), password(pass), name("IRC21")
{
	commands["PASS"] = &Server::cmd_pass;
	commands["USER"] = &Server::cmd_user;
	commands["NICK"] = &Server::cmd_nick;
	commands["PRIVMSG"] = &Server::cmd_privmsg;
	commands["NOTICE"] = &Server::cmd_privmsg;
	commands["AWAY"] = &Server::cmd_away;
	commands["PING"] = &Server::cmd_ping;
	commands["PONG"] = &Server::cmd_pong;
	commands["JOIN"] = &Server::cmd_join;
	commands["KICK"] = &Server::cmd_kick;
	commands["PART"] = &Server::cmd_part;
	commands["QUIT"] = &Server::cmd_quit;
	commands["ONLINE"] = &Server::cmd_online;
	commands["ISON"] = &Server::cmd_ison;
	commands["TOPIC"] = &Server::cmd_topic;

	users.push_back(new User(-1));
	users[0]->set_nick("MyBot");
	users[0]->set_auth_status(true);
}

Server::~Server()
{
	for (size_t i = 0; i < users.size(); i++)
		delete users[i];
	users.clear();
	for (size_t i = 0; i < channels.size(); i++)
		delete channels[i];
	channels.clear();
}

void	Server::create_socket()
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		std::cout << "Failed to create socket. errno: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
}

void	Server::bind_socket()
{
	const int trueFlag = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) < 0)
	{
		std::cout << "setsockopt failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sockaddr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
	{
		std::cout << "Failed to bind to port " << port << ". errno: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
}

void	Server::listen_socket()
{
	if (listen(sockfd, 128) < 0)
	{
		std::cout << "Failed to listen on socket. errno: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
}

void	Server::grab_connection()
{
	size_t addrlen = sizeof(sockaddr);
	int connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
	if (connection >= 0)
	{
		char	host[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(sockaddr.sin_addr), host, INET_ADDRSTRLEN);
		struct pollfd	pfd;
		pfd.fd = connection;
		pfd.events = POLLIN;
		pfd.revents = 0;
		userFDs.push_back(pfd);
		users.push_back(new User(userFDs[userFDs.size() - 1].fd));
	}
}

/*
	Отслеживает изменения по fd'шникам пользователей
	При изменении (вводе данных) забирает данные в буфер и отправляет в cmd_handler
*/
int		Server::process_messages()
{
	int	pret = poll(userFDs.data(), userFDs.size(), timeout);

    std::string	message;
	char	buffer[1024];
	int		readed;

	if (pret != 0)
	{
		for (size_t i = 0; i < userFDs.size(); i++)
		{
			if (userFDs[i].revents & POLLIN)
			{
				readed = 0;
				memset(buffer, 0, sizeof(buffer));
				while ((readed = recv(userFDs[i].fd, buffer, 1023, 0)) > 0)
				{
					buffer[readed] = '\0';
					message += buffer;
					memset(buffer, 0, sizeof(buffer));
					if (message.find('\n') != std::string::npos)
						break;
				}
				if (message.size() > 1) {
					User *usr_ptr = find_user_by_fd(userFDs[i].fd);
					cmd_handler(message, usr_ptr);
				}
				message.clear();
			}
			userFDs[i].revents = 0;
		}
	}
	return (0);
}

void	Server::check_users(){
	char *buf;
	size_t retval;

	for (size_t i = 0; i < users.size(); i++) {
		retval = recv(users[i]->get_fd(), &buf, 1, MSG_PEEK | MSG_DONTWAIT);
		if (retval == 0) {
			users[i]->set_auth_status(false);
			users[i]->set_nick_status(false);
			users[i]->set_pass_status(false);
			users[i]->set_username_status(false);
			close(users[i]->get_fd());
		}
		retval = 0;
	}
}

void	Server::cmd_handler(std::string input, User *cmd_init)
{
	std::vector<std::string>	commands;

	tokenize(input, '\n', commands);
	
	for (size_t i = 0; i < commands.size(); i++)
		execute_command(commands[i], cmd_init);
	commands.clear();
}

void	Server::execute_command(std::string cmd, User *cmd_init)
{
	Command		to_execute(cmd);
	std::string	command = to_execute.get_cmd();
	int			response = 0;

	if (cmd.length() > 1)
	{
		to_execute.show_cmd();
	
		if (!(cmd_init->get_auth_status() == true) && command != "PASS" && command != "NICK"
			&& command != "USER" && command != "QUIT")
				send_response(to_execute, name, cmd_init, ERR_NOTREGISTERED);
		else
		{
			if (commands[command] == 0)
				send_response(to_execute, name, cmd_init, ERR_UNKNOWNCOMMAND);
			else {
				response = (this->*(commands.at(command)))(to_execute, cmd_init);
				if (response != 0)
					send_response(to_execute, name, cmd_init, response);
			}
		}
	}
}

int		Server::cmd_pass(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();

	if (to_execute.get_num_of_args() != 1)
		return 461;

	if (arguments[0][0] == ':')
		arguments[0].erase(0);
	else
		return 461;

	if (password.compare(arguments[0]))
		cmd_init->set_pass_status(true);

	arguments.clear();
	return 0;
}

int		Server::cmd_user(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();

	if (to_execute.get_num_of_args() != 4)
		return 461;
	if (cmd_init->get_auth_status() == true)
		return 462;

	cmd_init->set_username(arguments[0]);
	cmd_init->set_address(arguments[2]);
	if (arguments[3][0] == ':')
		arguments[3].erase(0, 1);
	if (arguments[3][0] == '1')
		arguments[3].erase(0, 1);
	if (arguments[3][0] == ',')
		arguments[3].erase(0, 1);
	if (arguments[3][0] == '1')
		arguments[3].erase(0, 1);
	if (arguments[3][0] == '1')
		arguments[3].erase(0, 1);
	cmd_init->set_realname(arguments[3]);
	cmd_init->set_username_status(true);

	if (cmd_init->get_auth_status() == false && cmd_init->autorization_check() == true) {
		cmd_init->set_auth_status(true);
		send_motd(to_execute, cmd_init);
	}
	arguments.clear();
	return 0;
}

int		Server::cmd_nick(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();

	if (arguments.size() == 0)
		return 431;

	if (arguments[0].length() >= 9 || nick_is_valid(arguments[0]) == false)
		return 432;

	if (find_user_by_nick(arguments[0]) != NULL)
		if (find_user_by_nick(arguments[0])->get_auth_status() == true)
			return 433;

	cmd_init->set_nick(arguments[0]);
	cmd_init->set_nick_status(true);

	if (cmd_init->get_auth_status() == false && cmd_init->autorization_check() == true) {
		cmd_init->set_auth_status(true);
		send_motd(to_execute, cmd_init);
	}

	arguments.clear();
	return 0;
}

/*
	Отправляет сообщение
*/
int		Server::cmd_privmsg(Command to_execute, User *cmd_init)
{
	std::vector<User *>			message_for;
	std::vector<Channel *>		ch_message_for;
	std::vector<std::string>	arguments = to_execute.get_args();
	std::string					header;
	std::string					to_send;
	std::string					away_msg;
	bool						notice = false;
	int							i = 0;


	if (to_execute.get_cmd().compare("NOTICE") == 0)
		notice = true;

	// Формируем список получателей
	while (i < to_execute.get_num_of_args() && arguments[i][0] != ':') {
		if (*(arguments[i].end() - 1) == ',')
			arguments[i].erase(arguments[i].end() - 1);
		
		User*	user_ptr = find_user_by_nick(arguments[i]);
		if (user_ptr && user_ptr->get_auth_status())
			message_for.push_back(user_ptr);
		i++;
	}

	// Формируем список каналов для отправки
	i = 0;
	while (i < to_execute.get_num_of_args() && arguments[i][0] != ':') {
		if (*(arguments[i].end() - 1) == ',')
			arguments[i].erase(arguments[i].end() - 1);

		Channel*	channel_ptr = find_channel_by_name(arguments[i]);
		if (channel_ptr)
			if (channel_ptr->is_in_channel(cmd_init) == true)
				ch_message_for.push_back(channel_ptr);
		i++;
	}
	if (message_for.size() == 0 && ch_message_for.size() == 0)
		return ERR_NORECIPIENT;

	// Убираем двоеточие у первого слова сообщения
	if (arguments.size() > 1 && arguments[i][0] == ':')
		arguments[i] = arguments[i].erase(arguments[i].find(':'), 1);
	else
		return ERR_NOTEXTTOSEND;

	// Формируем сообщение
	while (i < to_execute.get_num_of_args()){
		to_send += arguments[i] + " ";
		i++;
	}
 	to_send += "\n";

	// Рассылаем сообщения в каналы
	for (size_t i = 0; i < ch_message_for.size(); i++)
		ch_message_for[i]->send_message_to_channel(to_send, this, notice, cmd_init);
	
	// Рассылаем сообщения в сообщение
	for (size_t j = 0; j < message_for.size(); j++) {
		if (message_for[j]->get_nick() == "MyBot"){
			weather_bot->get_command(to_execute, cmd_init, this);
			continue;
		}
		if (notice == false)
			header += ":" + cmd_init->get_nick() + "!" + cmd_init->get_username() + "@" + cmd_init->get_address() + " PRIVMSG " + message_for[j]->get_nick() + " :";
		else
			header += ":" + cmd_init->get_nick() + "!" + cmd_init->get_username() + "@" + cmd_init->get_address() + " NOTICE " + message_for[j]->get_nick() + " :";
		send_string_to_user(message_for[j], header);
		send_string_to_user(message_for[j], to_send);
		header.clear();

		// Обработка AWAY сообщения
		if (message_for[j]->get_away_on() == true && notice == false) {
			away_msg = ":" + name + " 301 " + cmd_init->get_nick() + " " + message_for[j]->get_nick() + " :" + message_for[j]->get_away_message();
			send_string_to_user(cmd_init, away_msg);
			away_msg.clear();
		}
	}
	to_send.clear();
	message_for.clear();
	ch_message_for.clear();
	arguments.clear();
	return 0;
}

/*
	Устанавливает сообщение-ответ на PRIVMSG
*/
int		Server::cmd_away(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();
	std::string	to_save;

	if (arguments.size() > 0)
	{
		cmd_init->set_away_on(true);

		if (arguments[0][0] == ':')
			arguments[0] = arguments[0].erase(arguments[0].find(':'), 1);
		for (int i = 0; i < to_execute.get_num_of_args(); i++)
			to_save += arguments[i] + " ";
 		to_save += "\n";

		cmd_init->set_away_message(to_save);
		arguments.clear();
		return RPL_NOWAWAY;
	}
	else
	{
		cmd_init->set_away_on(false);
		cmd_init->set_away_message("\0");
		arguments.clear();
		return RPL_UNAWAY;
	}
}

int		Server::cmd_quit(Command to_execute, User *cmd_init)
{
	(void) to_execute;

	send_string_to_user(cmd_init, "... Disconnected\n");
	cmd_init->set_auth_status(false);
	cmd_init->set_pass_status(false);
	cmd_init->set_nick_status(false);
	cmd_init->set_username_status(false);
	close (cmd_init->get_fd());

	return (0);
}

int		Server::cmd_ping(Command to_execute, User *cmd_init)
{
	if (to_execute.get_num_of_args() == 0)
		return (ERR_NOORIGIN);
	send_string_to_user(cmd_init, ":" + this->name + " PONG :" + to_execute.get_args()[0] + "\n");
	return 0;
}

int		Server::cmd_pong(Command to_execute, User *cmd_init){
	(void) to_execute;
	(void) cmd_init;

	return (0);
}

int			Server::cmd_ison(Command to_execute, User *cmd_init)
{
	send_response(to_execute, name, cmd_init, RPL_ISON);
	return (0);
}

int			Server::cmd_online(Command to_execute, User *cmd_init)
{
	(void) to_execute;
	std::string msg;

	for (size_t i = 0; i < users.size(); i++)
	{
		if (users[i]->get_auth_status() == true)
			msg += users[i]->get_nick() + " ";
	}
	msg += '\n';

	send_string_to_user(cmd_init, msg);
	return (0);
}

int		Server::cmd_join(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();

	if (arguments.size() < 1)
		return ERR_NEEDMOREPARAMS;

	std::string	channel_name = arguments[0];

	std::string	channel_key;
	if (arguments.size() > 1) {		// Пробуем найти key для канала
		channel_key = arguments[1];
	} else {						// Если key нет, то он пустая строка
		channel_key = "";
	}

	Channel	*channel = find_channel_by_name(channel_name);
	int		error_code;

	if (channel == NULL) {	// Создаём канал
		channels.push_back(new Channel(channel_name, cmd_init, channel_key));
	} else {				// Добавляем в канал если канал уже существует
		error_code = channel->add_user_to_channel(cmd_init, channel_key);
	}
	
	switch (error_code) {
	case ERR_BADCHANNELKEY:
		std::cout << cmd_init->get_nick() << " tried invalid key in " << channel_name << ", key = " << channel_key << std::endl;
		// send_response(to_execute, name, cmd_init, ERR_BADCHANNELKEY); // Тут должна быть отправка ошибки
		// break; //  Закомментировал, пока не разобрался, какой ответ отправлять
	case ERR_INVITEONLYCHAN:
		std::cout << cmd_init->get_nick() << " tried joining " << channel_name << ", which is invite only" << std::endl;
		// send_response(to_execute, name, cmd_init, ERR_INVITEONLYCHAN); // Тут должна быть отправка ошибки
		break; //  Закомментировал, пока не разобрался, какой ответ отправлять
	case ERR_USERONCHANNEL: // Пользователь уже в канале
		break ;
	default:
		send_response(to_execute, name, cmd_init, RPL_TOPIC);
		send_response(to_execute, name, cmd_init, RPL_NAMREPLY);
		send_response(to_execute, name, cmd_init, RPL_ENDRPL_NAMREPLY);
	}
	
	
	return (0);
}

int		Server::cmd_kick(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();
	Channel	*kick_from = find_channel_by_name(arguments[0]);
	User	*kick_this = find_user_by_nick(arguments[1]);
	std::string to_send;

	if (arguments.size() < 2)
		return ERR_NEEDMOREPARAMS;
	if (kick_from == NULL)
		return ERR_NOSUCHCHANNEL;
	if (kick_from->is_in_channel(kick_this) == false)
		return ERR_NOTONCHANNEL;
	if (kick_from->is_operator(cmd_init) == false)
		return ERR_CHANOPRIVSNEEDED;

	to_send += "KICK " + kick_from->get_name() + " " + kick_this->get_nick();
	for (size_t i = 2; i < arguments.size(); i++)
		to_send += " " + arguments[i];
	to_send += "\n";

	kick_from->delete_user_from_channel(kick_this);
	kick_from->send_message_to_channel(to_send, this, false, cmd_init);
	return (0);
}

int		Server::cmd_part(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();
	std::vector<Channel *>		kick_from;
	std::string to_send;

	for (size_t i = 0; i < arguments.size(); i++) {
		if (arguments[i][0] == ',')
			arguments[i].erase(1);
		if (arguments.size() < 1)
			return ERR_NEEDMOREPARAMS;
		if (find_channel_by_name(arguments[i]) == NULL)
			return ERR_NOSUCHCHANNEL;
		if (find_channel_by_name(arguments[i])->is_in_channel(cmd_init) == false)
			return ERR_NOTONCHANNEL;
		kick_from.push_back(find_channel_by_name(arguments[i]));
	}
	
	for (size_t i = 0; i < kick_from.size(); i++)
		kick_from[i]->delete_user_from_channel(cmd_init);

	return 0;
}

int		Server::cmd_topic(Command to_execute, User *cmd_init) {
	std::vector<std::string>	arguments = to_execute.get_args();

	if (arguments.size() < 1)
		return ERR_NEEDMOREPARAMS;
	
	Channel		*channel = find_channel_by_name(arguments[0]);
	if (channel == NULL || !channel->is_in_channel(cmd_init))
		return ERR_NOSUCHCHANNEL; // TODO: reply
	
	if (arguments.size() < 2) {
		send_response(to_execute, name, cmd_init, RPL_TOPIC); // FIXME: логика RPL_TOPIC в send_response
	} else {

		std::string		new_topic = "";
		// Убираем двоеточие у первого слова топика
		if (arguments[2].at(0) == ':')
			arguments[2] = arguments[2].erase(arguments[2].find(':'), 1);

		// Формируем топик
		for (size_t i = 2; i < to_execute.get_num_of_args(); i++) {
			new_topic += arguments[i];
		}

		int		err_code;
		err_code = channel->set_topic(cmd_init, new_topic);

		switch (err_code)
		{
		case ERR_CHANOPRIVSNEEDED:
			send_response(to_execute, name, cmd_init, ERR_CHANOPRIVSNEEDED);
			break ;
		default:
			send_response(to_execute, name, cmd_init, RPL_TOPIC);
		};
	}
	return 0;
}

/*
	Отправляет сообщение дня по FD'шнику
*/
void	Server::send_motd(Command to_execute, User *cmd_init)
{
	void (to_execute.get_cmd());

	send(cmd_init->get_fd(), ":IRC212 375 :- Message of the day - \n", 37, 0);
	send(cmd_init->get_fd(), ":IRC212 372 :- Welcome to school 21 fsteffan's server!\n", 55, 0);
	send(cmd_init->get_fd(), ":IRC212 376 :End of /MOTD command\n", 34, 0);
}

bool	Server::have_user(int new_user_fd)
{
	for (size_t i = 0; i < users.size(); i++) {
		if (users[i]->get_fd() == new_user_fd)
			return (true);
	}
	return (false);
}

User	*Server::find_user_by_fd(int user_fd)
{
	for (size_t i = 0; i < users.size(); i++)
		if (users[i]->get_fd() == user_fd)
			return users[i];
	return NULL;
}

User	*Server::find_user_by_nick(std::string user_nick)
{
	for (size_t i = 0; i < users.size(); i++)
	{
		if (users[i]->get_nick() == user_nick)
			return users[i];
	}
	return NULL;
}

Channel	*Server::find_channel_by_name(std::string channel_name)
{
	for (size_t i = 0; i < channels.size(); i++)
	{
		if (channels[i]->get_name() == channel_name)
			return channels[i];
	}
	return NULL;
}

void	Server::send_string_to_user(User *usr_ptr, std::string message) {
	send(usr_ptr->get_fd(), const_cast<char*>(message.c_str()), message.length(), 0);
}
