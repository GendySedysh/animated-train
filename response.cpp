#include "Server.hpp"

int	Server::send_response(const std::string from, User *cmd_init, int response,
								std::string arg1, std::string arg2, std::string arg3, std::string arg4)
{
	(void) arg2;
	(void) arg3;
	(void) arg4;

	std::stringstream	ss;
	ss << response;
	std::string	msg = ":" + from + " " + ss.str() + " " + cmd_init->get_nick() + " ";

	Channel	*channel = NULL;
	switch (response)
	{
	case RPL_ISON:
		msg += ":" + arg1 + "\n"; //arg1 = строка в которую записаны ники 
		break;
	case RPL_UNAWAY:
		msg += ":You are no longer marked as being away\n";
		break;
	case RPL_NOWAWAY:
		msg = ":You have been marked as being away\n";
		break;
	case RPL_MOTDSTART:
		msg = ":- Message of the day - \n";
		break;
	case RPL_MOTD:
		msg = ":- Welcome to school 21 fsteffan's server!\n";
		break;
	case RPL_ENDOFMOTD:
		msg += ":End of /MOTD command\n";
		break;
	case RPL_NOTOPIC:
		msg += arg1 + " :No topic set\n";	//arg1 = Имя канала
		break;
	case RPL_TOPIC:
		channel = find_channel_by_name(arg1);		//arg1 = Имя канала
		msg += arg1 + " :" + channel->get_topic() + "\n";
		break;
	case RPL_NAMREPLY:
		channel = find_channel_by_name(arg1);	//arg1 = Имя канала
		if (channel != NULL){
			std::vector<std::string> user_names = channel->get_user_name_vec();

			msg += "= " + arg1 + " :";
			for (size_t i = 0; i < user_names.size(); i++) {
				if (channel->is_operator(find_user_by_nick(user_names[i])))
					msg += "@" + user_names[i] + " ";
				else
					msg += user_names[i] + " ";
			}
		}
		msg += "\n";
		break;
	case RPL_ENDRPL_NAMREPLY:		//arg1 = Имя канала
		msg += arg1 + " :End of /NAMES list\n";
		break;
	case RPL_INVITING:
		msg += arg1 + " " + arg2 + "\n"; // arg1 = Имя канала, arg2 = Ник приглашенного пользователя
		break;
	case RPL_AWAY:
		// arg1 = Ник получателя сообщения, arg2 = его Away-сообщение	
		msg += arg1 + " :" + arg2 + "\n"; 
		break;
	case ERR_NONICKNAMEGIVEN:
		msg += ":No nickname given\n";
		break;
	case ERR_NICKNAMEINUSE:
		msg += arg1 +" :Nickname is already in use\n"; //arg1 = вводимый НИК
		break;
	case ERR_ERRONEUSNICKNAME:
		msg += arg1 + " :Erroneus nickname\n"; //arg1 = вводимый НИК
		break;
	case ERR_NEEDMOREPARAMS:
		msg += arg1 + " :Not enough parameters\n"; //arg1 = команда на ввод
		break;
	case ERR_ALREADYREGISTRED:
		msg += ":You may not reregister\n";
		break;
	case ERR_NORECIPIENT:
		msg += ":No recipient given ("+ arg1 + ")\n"; //arg1 = команда на ввод
		break;
	case ERR_NOORIGIN:
		msg += ":No origin specified\n";
		break;
	case ERR_NOTEXTTOSEND:
		msg += ":No text to send\n";
		break;
	case ERR_UNKNOWNCOMMAND:
		msg += arg1 + " :Unknown command\n"; //arg1 = команда на ввод
		break;
	case ERR_NOTREGISTERED:
		msg += ":You have not registered\n";
		break;
	case ERR_NOSUCHCHANNEL:
		msg += arg1 + " :No such channel\n"; //arg1 = Имя канала
		break;
	case ERR_NOTONCHANNEL:
		msg += arg1 + " :You're not on that channel\n"; //arg1 = Имя канала
		break;
	case ERR_CHANOPRIVSNEEDED:
		msg += arg1 + " :You're not channel operator\n"; //arg1 = Имя канала
		break;
	case ERR_USERONCHANNEL:
		// arg1 = Ник приглашенного пользователя, arg2 = Имя канала
		msg += " " + arg1 + " " + arg2 + " :is already on channel\n";
		break;
	case ERR_NOSUCHNICK:
		// arg1 = Ник пользователя
		msg += " " + arg1 + " :No such nick/channel\n";
		break;
	}
	send(cmd_init->get_fd(), msg.c_str(), msg.size(), 0);
	msg.clear();
	return (0);
}