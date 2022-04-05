#include "Server.hpp"

int		ft_isalpha(int c)
{
	if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122))
		return (1);
	return (0);
}

int		ft_isdigit(int c)
{
	if (c >= 48 && c <= 57)
		return (1);
	return (0);
}

int		ft_isspec(int c)
{
	if ((c >= 91 && c <= 94) || c == 45 || c == 123 || c == 125 || c == 0)
		return (1);
	return (0);
}

void	print_word_by_letters(std::string word)
{
	int i = 0;
	while (word[i])
	{
		std::cout << int(word[i]) << " ";
		i++;
	}
	std::cout << std::endl;
}

bool		nick_is_valid(std::string nick)
{
	for (size_t i = 0; i < nick.size(); i++){
		if (ft_isalpha(nick[i]) == 0 && ft_isdigit(nick[i]) == 0 && ft_isspec(nick[i]) == 0)
			return false;
	} 
	return (true);
}


int		ft_chan_uinvalid(int c)
{
	if (!(c >= 33 && c <= 126) || c == '\r' || c == '\n' || c == ' ' || c == '\0')
		return (0);
	return (1);
}

bool		channel_name_is_valid(std::string channel_name)
{
	if (channel_name[0] != '#' && channel_name[0] != '&')
		return false;
	for (size_t i = 0; i < channel_name.size(); i++){
		if (ft_chan_uinvalid(channel_name[i]) == 0)
			return false;
	} 
	return (true);
}

void tokenize(std::string &str, const char delim, std::vector<std::string> &out)
{
    size_t start;
    size_t end = 0;
	size_t dots;
	int c;

	dots = str.find(" :");
	if (dots != std::string::npos) {
		for (size_t i = 0; i < dots; i++)
		{
			if (str[i] == ',')
				str[i] = ' ';
		}
	}
	
 	while ((c = str.find('\r')) != -1)
        str.erase(str.begin() + c);

    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        out.push_back(str.substr(start, end - start));
    }
}