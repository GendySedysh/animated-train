#include "Server.hpp"

Server::Server(int port, char *pass): port(port), timeout(1), password(pass), name("IRC21")
{
}

Server::~Server()
{
	for (size_t i = 0; i < users.size(); i++)
		delete users[i];
	users.clear();
	for (size_t i = 0; i < chanels.size(); i++)
		delete chanels[i];
	chanels.clear();
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

    std::string	messege;
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
					messege += buffer;
					memset(buffer, 0, sizeof(buffer));
					if (messege.find('\n') != std::string::npos)
						break;
				}
				if (messege.size() > 1) {
					User *usr_ptr = find_user_by_fd(userFDs[i].fd);
					cmd_handler(messege, usr_ptr);
				}
				messege.clear();
			}
			userFDs[i].revents = 0;
		}
	}
	return (0);
}

void	Server::cmd_handler(std::string input, User *cmd_init)
{
	std::vector<std::string>	commands;

	tokenize(input, '\n', commands);
	
	for (size_t i = 0; i < commands.size(); i++)
		execute_command(commands[i], cmd_init);
	commands.clear();
}

int		Server::cmd_pass(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();

	if (to_execute.get_num_of_args() != 1)
		return 461;

	if (arguments[0][0] == ':')
		arguments[0].erase(0);

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
int		Server::cmd_privmsg(Command to_execute, User *cmd_init, bool notice)
{
	std::vector<User *>			messege_for;
	std::vector<Chanel *>		ch_messege_for;
	std::vector<std::string>	arguments = to_execute.get_args();
	std::string					header;
	std::string					to_send;
	std::string					away_msg;
	int							i = 0;

	// Формируем список получателей
	while (i < to_execute.get_num_of_args() && arguments[i][0] != ':') {
		if (*(arguments[i].end() - 1) == ',')
			arguments[i].erase(arguments[i].end() - 1);
		if (find_user_by_nick(arguments[i]) && find_user_by_nick(arguments[i])->get_auth_status())
			messege_for.push_back(find_user_by_nick(arguments[i]));
		i++;
	}

	// Формируем список каналов для отправки
	i = 0;
	while (i < to_execute.get_num_of_args() && arguments[i][0] != ':') {
		if (*(arguments[i].end() - 1) == ',')
			arguments[i].erase(arguments[i].end() - 1);
		if (find_chanel_by_name(arguments[i]))
			if ((find_chanel_by_name(arguments[i])->is_in_channel(cmd_init)) == true)
				ch_messege_for.push_back(find_chanel_by_name(arguments[i]));
		i++;
	}
	if (messege_for.size() == 0 && ch_messege_for.size() == 0)
		return ERR_NORECIPIENT;

	// Убираем двоеточие у первого слова сообщения
	if (arguments[i][0] == ':')
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
	for (size_t i = 0; i < ch_messege_for.size(); i++)
		ch_messege_for[i]->send_messege_to_chanel(to_send, this, notice, cmd_init);
	
	// Рассылаем сообщения в сообщение
	for (size_t j = 0; j < messege_for.size(); j++) {
		std::cout << messege_for[j]->get_nick() << std::endl;
		print_word_by_letters(messege_for[j]->get_nick());
		if (notice == false)
			header += ":" + cmd_init->get_nick() + "!" + cmd_init->get_username() + "@" + cmd_init->get_address() + " PRIVMSG " + messege_for[j]->get_nick() + " :";
		else
			header += ":" + cmd_init->get_nick() + "!" + cmd_init->get_username() + "@" + cmd_init->get_address() + " NOTICE " + messege_for[j]->get_nick() + " :";
		send_string_to_user(messege_for[j], header);
		send_string_to_user(messege_for[j], to_send);
		header.clear();

		// Обработка AWAY сообщения
		if (messege_for[j]->get_away_on() == true && notice == false) {
			away_msg = ":" + name + " 301 " + cmd_init->get_nick() + " " + messege_for[j]->get_nick() + " :" + messege_for[j]->get_away_massage();
			send_string_to_user(cmd_init, away_msg);
			away_msg.clear();
		}
	}
	to_send.clear();
	messege_for.clear();
	ch_messege_for.clear();
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

		cmd_init->set_away_massage(to_save);
		arguments.clear();
		return RPL_NOWAWAY;
	}
	else
	{
		cmd_init->set_away_on(false);
		cmd_init->set_away_massage("\0");
		arguments.clear();
		return RPL_UNAWAY;
	}
}

void	Server::cmd_quit(Command to_execute, User *cmd_init)
{
	(void) to_execute;

	send_string_to_user(cmd_init, "... Disconnected\n");
	cmd_init->set_auth_status(false);
	cmd_init->set_pass_status(false);
	cmd_init->set_nick_status(false);
	cmd_init->set_username_status(false);
	close (cmd_init->get_fd());
}

int		Server::cmd_ping(Command to_execute, User *cmd_init)
{
	if (to_execute.get_num_of_args() == 0)
		return (ERR_NOORIGIN);
	send_string_to_user(cmd_init, ":" + this->name + " PONG :" + to_execute.get_args()[0] + "\n");
	return 0;
}

void		Server::cmd_ison(Command to_execute, User *cmd_init)
{
	send_response(to_execute, name, cmd_init, RPL_ISON);
}

void		Server::cmd_online(User *cmd_init)
{
	std::string msg;

	for (size_t i = 0; i < users.size(); i++)
	{
		if (users[i]->get_auth_status() == true)
			msg += users[i]->get_nick() + " ";
	}
	msg += '\n';

	send_string_to_user(cmd_init, msg);
}

int		Server::cmd_join(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();

	if (arguments.size() < 1)
		return ERR_NEEDMOREPARAMS;
			
	Chanel	*chanel = find_chanel_by_name(to_execute.get_args()[0]);
	
	if (chanel == NULL)	// Создаём канал
		chanels.push_back(new Chanel(to_execute.get_args()[0], cmd_init));
	else				// Добавляем в канал если канал уже существует
		chanel->add_user_to_channel(cmd_init);
	send_response(to_execute, name, cmd_init, RPL_TOPIC);
	send_response(to_execute, name, cmd_init, RPL_NAMREPLY);
	send_response(to_execute, name, cmd_init, RPL_ENDRPL_NAMREPLY);
	return (0);
}

int		Server::cmd_kick(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();
	Chanel	*kick_from = find_chanel_by_name(arguments[0]);
	User	*kick_this = find_user_by_nick(arguments[1]);
	std::string to_send;

	if (arguments.size() < 2)
		return ERR_NEEDMOREPARAMS;
	if (kick_from == NULL)
		return ERR_NOSUCHCHANNEL; //
	if (kick_from->is_in_channel(kick_this) == false)
		return ERR_NOTONCHANNEL; //
	if (kick_from->is_operator(cmd_init) == false)
		return ERR_CHANOPRIVSNEEDED; //

	to_send += "KICK " + kick_from->get_name() + " " + kick_this->get_nick();
	for (size_t i = 2; i < arguments.size(); i++)
		to_send += " " + arguments[i];
	to_send += "\n";

	kick_from->delete_user_from_channel(kick_this);
	kick_from->send_messege_to_chanel(to_send, this, false, cmd_init);
	return (0);
}

int		Server::cmd_part(Command to_execute, User *cmd_init)
{
	std::vector<std::string>	arguments = to_execute.get_args();
	std::vector<Chanel *>		kick_from;
	std::string to_send;

	for (size_t i = 0; i < arguments.size(); i++) {
		if (arguments[i][0] == ',')
			arguments[i].erase(1);
		if (arguments.size() < 1)
			return ERR_NEEDMOREPARAMS;
		if (find_chanel_by_name(arguments[i]) == NULL)
			return ERR_NOSUCHCHANNEL;
		if (find_chanel_by_name(arguments[i])->is_in_channel(cmd_init) == false)
			return ERR_NOTONCHANNEL;
		kick_from.push_back(find_chanel_by_name(arguments[i]));
	}
	
	for (size_t i = 0; i < kick_from.size(); i++)
		kick_from[i]->delete_user_from_channel(cmd_init);

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

void	Server::execute_command(std::string cmd, User *cmd_init)
{
	Command		to_execute(cmd);
	int			response = 0;

	if (cmd.length() > 1)
	{
		to_execute.show_cmd();
	
		if (cmd_init->get_auth_status() == false)
		{
			if (to_execute.get_cmd().compare("PASS") == 0)
				response = cmd_pass(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("NICK") == 0)
				response = cmd_nick(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("USER") == 0)
				response = cmd_user(to_execute, cmd_init);
			else
				send_response(to_execute, name, cmd_init, ERR_NOTREGISTERED);
		}
		else
		{
			if (to_execute.get_cmd().compare("PASS") == 0)
				response = cmd_pass(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("NICK") == 0)
				response = cmd_nick(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("USER") == 0)
				response = cmd_user(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("PRIVMSG") == 0)
				response = cmd_privmsg(to_execute, cmd_init, false);
			else if (to_execute.get_cmd().compare("NOTICE") == 0)
				response = cmd_privmsg(to_execute, cmd_init, true);
			else if (to_execute.get_cmd().compare("AWAY") == 0)
				response = cmd_away(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("ISON") == 0)
				cmd_ison(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("PING") == 0)
				response = cmd_ping(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("ONLINE") == 0)
				cmd_online(cmd_init);
			else if (to_execute.get_cmd().compare("QUIT") == 0)
				cmd_quit(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("JOIN") == 0)
				response = cmd_join(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("KICK") == 0)
				response = cmd_kick(to_execute, cmd_init);
			else if (to_execute.get_cmd().compare("PART") == 0)
				response = cmd_part(to_execute, cmd_init);
			else
				send_response(to_execute, name, cmd_init, ERR_UNKNOWNCOMMAND);
		}
	}
	if (response != 0)
		send_response(to_execute, name, cmd_init, response);
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

Chanel	*Server::find_chanel_by_name(std::string chanel_name)
{
	for (size_t i = 0; i < chanels.size(); i++)
	{
		if (chanels[i]->get_name() == chanel_name)
			return chanels[i];
	}
	return NULL;
}

void	Server::send_string_to_user(User *usr_ptr, std::string massage) {
	send(usr_ptr->get_fd(), const_cast<char*>(massage.c_str()), massage.length(), 0);
}

void	Server::send_response(Command to_execute, const std::string from, User *cmd_init, int responce)
{
	std::stringstream	ss;
	ss << responce;
	std::string	msg = ":" + from + " " + ss.str() + " " + cmd_init->get_nick() + " ";
	// msg += ss.str() + " " + cmd_init->get_nick() + " ";

	std::vector<std::string>	arguments = to_execute.get_args();
	Chanel *chanel = find_chanel_by_name(arguments[0]);

	switch (responce)
	{
	case RPL_ISON:								//ISON
		msg += ":";
		for (size_t i = 0; i < arguments.size(); i++)
		{
			if (find_user_by_nick(arguments[i]) != NULL)
				msg += arguments[i] + " ";
		}
		msg += "\n";
		break;
	case RPL_UNAWAY:							//AWAY
		msg += ":You are no longer marked as being away\n";
		break;
	case RPL_NOWAWAY:							//AWAY
		msg = ":You have been marked as being away\n";
		break;
	case RPL_MOTDSTART:							//MOTD
		msg = ":- Message of the day - \n";
		break;
	case RPL_MOTD:								//MOTD
		msg = ":- Welcome to school 21 fsteffan's server!\n";
		break;
	case RPL_ENDOFMOTD:							//MOTD
		msg += ":End of /MOTD command\n";
		break;
	case RPL_TOPIC:
		msg += chanel->get_name() + " :No topic set\n";
		break;
	case RPL_NAMREPLY:
		if (chanel != NULL){
			std::vector<std::string> user_names = chanel->get_user_name_vec();

			msg += chanel->get_name() + " :";
			for (size_t i = 0; i < user_names.size(); i++) {
				msg += "@" + user_names[i] + " ";
			}
		}
		msg += "\n";
		break;
	case RPL_ENDRPL_NAMREPLY:
		msg += chanel->get_name() + " :End of /NAMES list\n";
		break;
	case ERR_NONICKNAMEGIVEN:
		msg += ":No nickname given\n";
		break;
	case ERR_NICKNAMEINUSE:
		msg += arguments[0] +" :Nickname is already in use\n";
		break;
	case ERR_ERRONEUSNICKNAME:
		msg += arguments[0] + " :Erroneus nickname\n";
		break;
	case ERR_NEEDMOREPARAMS:
		msg += to_execute.get_cmd() + " :Not enough parameters\n";
		break;
	case ERR_ALREADYREGISTRED:
		msg += ":You may not reregister\n";
		break;
	case ERR_NORECIPIENT:
		msg += ":No recipient given ("+ to_execute.get_cmd() + ")\n";
		break;
	case ERR_NOORIGIN:
		msg += ":No origin specified\n";
		break;
	case ERR_NOTEXTTOSEND:
		msg += ":No text to send\n";
		break;
	case ERR_UNKNOWNCOMMAND:
		msg += to_execute.get_cmd() + " :Unknown command\n";
		break;
	case ERR_NOTREGISTERED:
		msg += ":You have not registered\n";
		break;
	case ERR_NOSUCHCHANNEL:
		msg += chanel->get_name() + " :No such channel\n";
		break;
	case ERR_NOTONCHANNEL:
		msg += chanel->get_name() + " :You're not on that channel\n";
		break;
	case ERR_CHANOPRIVSNEEDED:
		msg += chanel->get_name() + " :You're not channel operator\n";
		break;
	}
	send(cmd_init->get_fd(), msg.c_str(), msg.size(), 0);
	msg.clear();
}