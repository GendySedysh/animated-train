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
	commands["INVITE"] = &Server::cmd_invite;
	commands["MODE"] = &Server::cmd_mode;

	flags['i'] = CH_INVITEONLY;
	flags['n'] = CH_NOMSGFROMOUT;
	flags['l'] = CH_LIMITED;
	flags['k'] = CH_KEYSTATUS;
	flags['t'] = CH_TOPICSETOP;
	flags['o'] = CH_OPERATOR;

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
			}
			message.clear();
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

void	Server::check_channels(){
	for (size_t i = 0; i < channels.size(); i++) {
		channels[i]->delete_offline_users();
		if (channels[i]->get_user_name_vec().size() == 0)
			channels[i]->set_name("deleted");
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

	if (cmd.length() > 1)
	{
		to_execute.show_cmd();
	
		if (!(cmd_init->get_auth_status() == true) && command != "PASS" && command != "NICK"
			&& command != "USER" && command != "QUIT")
				send_response(name, cmd_init, ERR_NOTREGISTERED, cmd_init->get_nick(), "0", "0", "0");
		else
		{
			if (commands[command] == 0)
				send_response(name, cmd_init, ERR_UNKNOWNCOMMAND, command, "0", "0", "0");
			else
				(this->*(commands.at(command)))(to_execute, cmd_init);
		}
	}
}

int		Server::cmd_pass(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();

	if (to_execute.get_num_of_args() != 1)
		return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");

	if (arguments[0][0] == ':')
		arguments[0].erase(0);
	else 
		return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");

	if (password.compare(arguments[0]))
		cmd_init->set_pass_status(true);

	arguments.clear();
	return 0;
}

int		Server::cmd_user(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();

	if (to_execute.get_num_of_args() != 4)
		return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");

	if (cmd_init->get_auth_status() == true)
		return send_response(name, cmd_init, ERR_ALREADYREGISTRED, "0", "0", "0", "0");

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
		return send_response(name, cmd_init, ERR_NONICKNAMEGIVEN, "0", "0", "0", "0");

	if (arguments[0].length() >= 9 || nick_is_valid(arguments[0]) == false)
		return send_response(name, cmd_init, ERR_ERRONEUSNICKNAME, arguments[0], "0", "0", "0");

	if (find_user_by_nick(arguments[0]) != NULL)
		if (find_user_by_nick(arguments[0])->get_auth_status() == true)
			return send_response(name, cmd_init, ERR_NICKNAMEINUSE, arguments[0], "0", "0", "0");

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
			if (channel_ptr->is_in_channel(cmd_init) == true || channel_ptr->is_reachable_from_outside() != true) //ТУТ МОЖНО СДЕЛАТЬ ПРОВЕРКУ НА MODE +n в
				ch_message_for.push_back(channel_ptr);
		i++;
	}
	if (message_for.size() == 0 && ch_message_for.size() == 0)
		return send_response(name, cmd_init, ERR_NORECIPIENT, to_execute.get_cmd(), "0", "0", "0");

	// Убираем двоеточие у первого слова сообщения
	if (arguments.size() > 1 && arguments[i] != "") {
		if (arguments[i][0] == ':')
			arguments[i] = arguments[i].erase(0, 1);
	}
	else
		return send_response(name, cmd_init, ERR_NOTEXTTOSEND, "0", "0", "0", "0");

	// Формируем сообщение
	while (i < to_execute.get_num_of_args()){
		to_send += arguments[i] + " ";
		i++;
	}
 	to_send += "\n";

	// Рассылаем сообщения в каналы
	for (size_t i = 0; i < ch_message_for.size(); i++)
		ch_message_for[i]->send_message_to_channel(to_send, this, notice, cmd_init);
	
	// Рассылаем сообщения в личку
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

int		Server::cmd_mode(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();
	Channel						*channel_to_mode;

	if (to_execute.get_num_of_args() < 1)
		return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");

	channel_to_mode = find_channel_by_name(arguments[0]);

	if (channel_to_mode == NULL)
		return send_response(name, cmd_init, ERR_NOSUCHCHANNEL, arguments[0], "0", "0", "0");

	if (channel_to_mode->is_operator(cmd_init) == false)
		return send_response(name, cmd_init, ERR_CHANOPRIVSNEEDED, arguments[0], "0", "0", "0");

	if (to_execute.get_num_of_args() == 1) { //ТУТ Должна отправляться строка-сообщение о активных флагах
		std::string to_send;

		to_send = ":" + this->name + " 324 " + cmd_init->get_nick() + " " + channel_to_mode->get_name() +
					" " + channel_to_mode->flag_status() + "\n";
		send_string_to_user(cmd_init, to_send);
		return 0;
	}
	else { // ТУТ должна проводиться обработка флагов
		if (arguments[1].size() == 2)
		{
			std::string		to_send;
			std::string		to_add = " ";
			char			operand = arguments[1][0];
			unsigned int	flag = flags[arguments[1][1]];

			if (operand != '+' && operand != '-')
				return send_response(name, cmd_init, ERR_UNKNOWNMODE, arguments[1], "0", "0", "0");				

			switch (flag)
			{
			case CH_OPERATOR:
				if (arguments[2] != ""){
					User *to_mod = find_user_by_nick(arguments[2]);
					
					if (to_mod == NULL || channel_to_mode->is_in_channel(to_mod) == false)
						return send_response(name, cmd_init, ERR_NOSUCHNICK, arguments[2], "0", "0", "0");
					
					if (operand == '+')
						channel_to_mode->add_user_to_channel_operator(to_mod);
					else
						channel_to_mode->delete_user_from_channel_operator(to_mod);
					to_add = to_mod->get_nick();
				}
				else
					return send_response(name, cmd_init, ERR_UNKNOWNMODE, arguments[1], "0", "0", "0");
				break;
			case CH_LIMITED:
				if (operand == '+') {
					if (arguments[2] != "") {
						int new_limit = atoi(arguments[2].c_str());
						channel_to_mode->set_limit(new_limit);
						to_add = arguments[2];
					}
					else
						return send_response(name, cmd_init, ERR_UNKNOWNMODE, arguments[1], "0", "0", "0");
				}
				else if (operand == '-') {}
				else
					return send_response(name, cmd_init, ERR_UNKNOWNMODE, arguments[1], "0", "0", "0");
				break;
			case CH_KEYSTATUS:
				if (operand == '+') {
					if (arguments[2] != "") {
						channel_to_mode->set_key(arguments[2]);
						to_add = arguments[2];
					}
					else
						return send_response(name, cmd_init, ERR_UNKNOWNMODE, arguments[1], "0", "0", "0");
				}
				else if (operand == '-') {}
				else
					return send_response(name, cmd_init, ERR_UNKNOWNMODE, arguments[1], "0", "0", "0");
				break;
			case CH_INVITEONLY:
				break;
			case CH_TOPICSETOP:
				break;
			case CH_NOMSGFROMOUT:
				break;
			default:
				return send_response(name, cmd_init, ERR_UNKNOWNMODE, arguments[1], "0", "0", "0");
			}

			if (operand == '+')
				channel_to_mode->set_flag(flag);
			if (operand == '-')
				channel_to_mode->unset_flag(flag);

			to_send = ":" + cmd_init->get_info_string() + " MODE " + arguments[0] + " " + operand + arguments[1][1];
			if (flag == CH_OPERATOR || flag == CH_LIMITED || flag == CH_KEYSTATUS)
				to_send += " " + to_add;
			to_send += "\n";
			channel_to_mode->send_string_to_channel(to_send);
			return 0;
		}
		else
			return send_response(name, cmd_init, ERR_UNKNOWNMODE, arguments[1], "0", "0", "0");
	}
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
		send_response(name, cmd_init, RPL_NOWAWAY, "0", "0", "0", "0");
	}
	else
	{
		cmd_init->set_away_on(false);
		cmd_init->set_away_message("\0");
		arguments.clear();
		send_response(name, cmd_init, RPL_UNAWAY, "0", "0", "0", "0");
	}
	return (0);
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
		return send_response(name, cmd_init, ERR_NOORIGIN, "0", "0", "0", "0");

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
	std::vector<std::string> args = to_execute.get_args();
	std::string	to_send;

	for (size_t i = 0; i < args.size(); i++)
	{
		if (find_user_by_nick(args[i]) != NULL)
			to_send += args[i] + " ";
	}
	
	send_response(name, cmd_init, RPL_ISON, to_send, "0", "0", "0");
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
		return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");

	std::vector<std::string>	channel_names;
	std::vector<std::string>	keys;

	tokenize(arguments[0], ',', channel_names);
	if (arguments.size() >= 2)
		tokenize(arguments[1], ',', keys);

	for (size_t i = 0; i < channel_names.size(); i++) {
		std::string	channel_name = channel_names[i];

		std::string	channel_key;
		try {
			channel_key = keys.at(i); // Пробуем найти key для канала
		} catch (const std::out_of_range &e) {
			channel_key = "";  // Если key нет, то он пустая строка
		}

		if (channel_name_is_valid(channel_name) == false)
			return send_response(name, cmd_init, ERR_NOSUCHCHANNEL, channel_name, "0", "0", "0");

		Channel	*channel = find_channel_by_name(channel_name);
		int		error_code = 0;

		if (channel == NULL) {	// Создаём канал
			channel = new Channel(channel_name, cmd_init, channel_key);
			channels.push_back(channel);
		} else {				// Добавляем в канал если канал уже существует
			error_code = channel->add_user_to_channel(cmd_init, channel_key);
		}
		
		switch (error_code) {
		case ERR_CHANNELISFULL:
			return send_response(name, cmd_init, ERR_CHANNELISFULL, channel_name, "0", "0", "0");
			break;
		case ERR_BADCHANNELKEY:
			return send_response(name, cmd_init, ERR_BADCHANNELKEY, channel_name, "0", "0", "0");
			break;
		case ERR_INVITEONLYCHAN:
			return send_response(name, cmd_init, ERR_INVITEONLYCHAN, channel_name, "0", "0", "0");
			break;
		case ERR_USERONCHANNEL: // Пользователь уже в канале
			return send_response(name, cmd_init, ERR_USERONCHANNEL, cmd_init->get_nick(), channel_name, "0", "0");
			break ;
		default:
			send_string_to_user(cmd_init, ":" + cmd_init->get_nick() + "!" + cmd_init->get_username() // эта строка заставляет клиент открывать новое окно для канала
								+ "@" + cmd_init->get_address() + " JOIN :" + channel->get_name() + "\n");
			if (channel->get_topic().size() > 0) {
				send_response(name, cmd_init, RPL_TOPIC, channel->get_name(), "0", "0", "0");
			} else {
				send_response(name, cmd_init, RPL_NOTOPIC, channel->get_name(), "0", "0", "0");
			}
			send_response(name, cmd_init, RPL_NAMREPLY, channel->get_name(), "0", "0", "0");
			send_response(name, cmd_init, RPL_ENDRPL_NAMREPLY, channel->get_name(), "0", "0", "0");
		}
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
		return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");

	if (kick_from == NULL)
		return send_response(name, cmd_init, ERR_NOSUCHCHANNEL, arguments[0], "0", "0", "0");

	if (kick_from->is_in_channel(kick_this) == false)
		return send_response(name, cmd_init, ERR_NOTONCHANNEL, arguments[0], "0", "0", "0");

	if (kick_from->is_operator(cmd_init) == false)
		return send_response(name, cmd_init, ERR_CHANOPRIVSNEEDED, arguments[0], "0", "0", "0");

	to_send = ":" + cmd_init->get_info_string() + " KICK " + kick_from->get_name() + " " +
				kick_this->get_nick() + " " + cmd_init->get_nick() + " :";
	for (size_t i = 2; i < arguments.size(); i++)
		to_send += " " + arguments[i];
	to_send += "\n";


	std::vector<std::string> send_to = kick_from->get_user_name_vec();
	for (size_t i = 0; i < send_to.size(); i++)
		send_string_to_user(find_user_by_nick(send_to[i]), to_send);
	
	kick_from->delete_user_from_channel(kick_this);
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
			return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");

		if (find_channel_by_name(arguments[i]) == NULL)
			return send_response(name, cmd_init, ERR_NOSUCHCHANNEL, arguments[i], "0", "0", "0");

		if (find_channel_by_name(arguments[i])->is_in_channel(cmd_init) == false)
			return send_response(name, cmd_init, ERR_NOTONCHANNEL, arguments[i], "0", "0", "0");

		kick_from.push_back(find_channel_by_name(arguments[i]));
	}

	for (size_t i = 0; i < kick_from.size(); i++) {
		to_send = ":" + cmd_init->get_info_string() + " PART " + kick_from[i]->get_name() + "\n";
		kick_from[i]->send_string_to_channel(to_send);
		kick_from[i]->delete_user_from_channel(cmd_init);
	}

	return 0;
}

int		Server::cmd_invite(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();
	if (arguments.size() < 2)
		return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");

	// arguments[0] - Nickname
	// arguments[1] - channel

	User	*reciever = find_user_by_nick(arguments[0]);
	if (reciever == NULL) {
		return send_response(name, cmd_init, ERR_NOSUCHNICK, arguments[0], "0", "0", "0");
	}

	Channel	*channel = find_channel_by_name(arguments[1]);
	if (channel == NULL) {
		return send_response(name, cmd_init, ERR_NOSUCHCHANNEL, to_execute.get_cmd(), "0", "0", "0");
	}

	int	res = channel->invite(cmd_init, reciever);

	switch (res) {
	case ERR_CHANOPRIVSNEEDED:
		return send_response(name, cmd_init, ERR_CHANOPRIVSNEEDED, channel->get_name(), "0", "0", "0");
	case ERR_USERONCHANNEL:
		return send_response(name, cmd_init, ERR_USERONCHANNEL, reciever->get_nick(), channel->get_name(), "0", "0");
	default:  // RPL_INVITING
		send_response(name, cmd_init, RPL_INVITING, channel->get_name(), reciever->get_nick(), "0", "0"); // todo
		if (reciever->get_away_on()) {
			send_response(name, cmd_init, RPL_AWAY, reciever->get_nick(), reciever->get_away_message(), "0", "0");
		}
	}
	return 0;
}

int		Server::cmd_topic(Command to_execute, User *cmd_init) {
	std::vector<std::string>	arguments = to_execute.get_args();

	if (arguments.size() < 1)
		return send_response(name, cmd_init, ERR_NEEDMOREPARAMS, to_execute.get_cmd(), "0", "0", "0");
	
	Channel		*channel = find_channel_by_name(arguments[0]);
	if (channel == NULL)
		return send_response(name, cmd_init, ERR_NOSUCHCHANNEL, arguments[0], "0", "0", "0");

	else if (!channel->is_in_channel(cmd_init))
		return send_response(name, cmd_init, ERR_NOTONCHANNEL, arguments[0], "0", "0", "0");
	
	if (arguments.size() < 2) {
		send_response(name, cmd_init, RPL_TOPIC, channel->get_name(), "0", "0", "0");
	} else {

		std::string		new_topic = "";
		// Убираем двоеточие у первого слова топика
		if (arguments[1].at(0) == ':')
			arguments[1] = arguments[1].erase(arguments[1].find(':'), 1);

		// Формируем топик
		for (int i = 1; i < to_execute.get_num_of_args(); i++) {
			std::string	space = (i < to_execute.get_num_of_args() - 1) ? " " : "";
			new_topic += arguments[i] + space;
		}

		int		err_code;
		err_code = channel->set_topic(cmd_init, new_topic);

		if (err_code == ERR_CHANOPRIVSNEEDED) { // TODO: test it when user is not operator
			send_response(name, cmd_init, ERR_CHANOPRIVSNEEDED, channel->get_name(), "0", "0", "0");
		} else {
			std::string msg = "TOPIC " + channel->get_name() + " :" + channel->get_topic() + "\n";
			channel->send_message_to_channel(msg, this, false, cmd_init);
		}
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
