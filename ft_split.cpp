#include "Server.hpp"

int				ft_length(const char *str)
{
	int i = 0;

	while (str[i])
		i++;
	return i;
}

static	int		is_delimiter(char char_in, char delim)
{
	return (char_in == delim);
}

char			*fill_in_word(char const *str, char c)
{
	char	*word;
	int		i;

	i = 0;
	while (str[i] && !is_delimiter(str[i], c))
		i++;
	if (!(word = (char *)malloc(sizeof(char) * (i + 1))))
		return (0);
	i = 0;
	while (str[i] && !is_delimiter(str[i], c))
	{
		word[i] = str[i];
		i++;
	}
	word[i] = '\0';
	return (word);
}

static void		free_all(char **str)
{
	int		i;

	i = 0;
	while (str[i])
	{
		free(str[i]);
		i++;
	}
	free(str);
}

static char		**fill_splitted(char **str, char const *s, char c)
{
	int		i;

	i = 0;
	while (*s)
	{
		while (*s && is_delimiter(*s, c))
			s++;
		if (*s && !is_delimiter(*s, c))
		{
			if (!(str[i] = fill_in_word(s, c)))
			{
				free_all(str);
				return (0);
			}
			i++;
			while (*s && !is_delimiter(*s, c))
				s++;
		}
		if (*s)
			s++;
	}
	str[i] = NULL;
	return (str);
}

char			**ft_split(char const *s, char c)
{
	char	**str;
	int		i;
	int		split_num;

	split_num = 0;
	i = 0;
	while (s[i])
	{
		while (s[i] && is_delimiter(s[i], c))
			i++;
		if (s[i] && !is_delimiter(s[i], c))
		{
			split_num++;
			while (s[i] && !is_delimiter(s[i], c))
				i++;
		}
	}
	if (!(str = (char**)malloc((split_num + 1) * sizeof(char*))))
		return (0);
	return (fill_splitted(str, s, c));
}

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
	// char *spec_symbols = "-[]\\^{}"; // 45, 91, 92, 93, 94, 123, 125 

	if ((c >= 91 && c <= 94) || c == 45 || c == 123 || c == 125)
		return (1);
	return (0);
}