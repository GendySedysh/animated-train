#ifndef COMMAND_HPP
# define COMMAND_HPP

#include "Server.hpp"

class Command
{
private:
	int							num_of_args;
	std::vector<std::string>	args;
	std::string					cmd;
	std::string					prefix;
	bool						have_prefix;

public:
	Command(const char *messege);
	~Command();

	int							get_num_of_args();
	std::string					get_cmd();
	std::vector<std::string>	get_args();
	std::string					get_prefix();
	void						show_cmd();
};

#endif