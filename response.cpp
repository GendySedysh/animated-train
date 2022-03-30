#include "Server.hpp"

void	Server::send_response(Command to_execute, const std::string from, User *cmd_init, int response)
{
	std::stringstream	ss;
	ss << response;
	std::string	msg = ":" + from + " " + ss.str() + " " + cmd_init->get_nick() + " ";


	std::vector<std::string>	arguments = to_execute.get_args();
	Channel *channel;

	if (arguments.size() > 0)
		channel = find_channel_by_name(arguments[0]);
	else
		channel = NULL;

	switch (response)
	{
	case RPL_ISON:
		msg += ":";
		for (size_t i = 0; i < arguments.size(); i++)
		{
			if (find_user_by_nick(arguments[i]) != NULL)
				msg += arguments[i] + " ";
		}
		msg += "\n";
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
		msg += channel->get_name() + " :No topic set\n";
		break;
	case RPL_TOPIC:
		msg += channel->get_name() + " :" + channel->get_topic() + "\n";
		break;
	case RPL_NAMREPLY:
		if (channel != NULL){
			std::vector<std::string> user_names = channel->get_user_name_vec();

			msg += channel->get_name() + " :";
			for (size_t i = 0; i < user_names.size(); i++) {
				msg += "@" + user_names[i] + " ";
			}
		}
		msg += "\n";
		break;
	case RPL_ENDRPL_NAMREPLY:
		msg += channel->get_name() + " :End of /NAMES list\n";
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
		msg += arguments[0] + " :No such channel\n";
		break;
	case ERR_NOTONCHANNEL:
		msg += channel->get_name() + " :You're not on that channel\n";
		break;
	case ERR_CHANOPRIVSNEEDED:
		msg += channel->get_name() + " :You're not channel operator\n";
		break;
	}
	send(cmd_init->get_fd(), msg.c_str(), msg.size(), 0);
	msg.clear();
}