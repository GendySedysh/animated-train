#include "Command.hpp"

Command::Command(std::string messege)
{
	std::vector<std::string> words;

	int i = 0;

	if (messege.length() > 1)
	{
		prefix = " ";

		tokenize(messege, ' ', words);
		if (messege[0] == ':'){
			have_prefix = true;
			prefix = words[0].erase(0, 1);
			cmd = words[1];
			num_of_args = words.size() - 2;
			i = 2;
		}
		else {
			have_prefix = false;
			cmd = words[0];
			num_of_args = words.size() - 1;
			i = 1;
		}
		while (i < int(words.size()))
		{
			args.push_back(words[i]);
			i++;
		}
		words.clear();
	}
}

Command::~Command()
{
	args.clear();
}

void	Command::show_cmd() {
	std::cout << "Command: " << this->cmd << std::endl;
	std::cout << "Prefix: " << this->prefix << "| Num of arsgs: " << num_of_args << std::endl;
	for (size_t i = 0; i < args.size(); i++)
		std::cout << args[i] << " ";
	std::cout << std::endl << std::endl;
}

int							Command::get_num_of_args() { return this->num_of_args; }
std::string					Command::get_cmd() { return this->cmd; }
std::vector<std::string>	Command::get_args() { return this->args; }
std::string					Command::get_prefix() { return this->prefix; }