#include "Bot.hpp"

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void	Bot::get_command(Command to_execute, User *cmd_init, Server *server)
{
	std::vector<std::string>	arguments = to_execute.get_args();
	std::string					header;

	header += ":MyBot!Megatron@0.0.0.0 PRIVMSG " + cmd_init->get_nick() + " :";
	if (arguments.size() < 2){
		server->send_string_to_user(cmd_init, header);
		server->send_string_to_user(cmd_init, "Введи название города\n");
		return ;
	}

	CURL *curl;
	CURLcode response;
	std::string readBuffer;

	size_t i;

	for (i = 0; i < arguments.size(); i++) {
		if (arguments[i][0] == ':') {
			std::cout << i << "___" << arguments[i] << std::endl;
			arguments[i].erase(0, 1);
			break;
		}
	}
	
	std::string MAIN_IP = "http://api.weatherapi.com/v1/current.json?key=6a5f189b0b7e44b8981180005222903&q=" + arguments[i] + "&lang=ru&aqi=no";
	const char *c = MAIN_IP.c_str();

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, c);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		response = curl_easy_perform(curl);

		if (response != CURLE_OK)
			printf("REQUEST FAILED\n");
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	weather_data(readBuffer, cmd_init, server);
}

void	Bot::weather_data(std::string text, User *cmd_init, Server *server)
{
	std::string					header;

	header += ":MyBot!Megatron@0.0.0.0 PRIVMSG " + cmd_init->get_nick() + " :";

	if (text.find("error") != std::string::npos) {
		server->send_string_to_user(cmd_init, header);
		server->send_string_to_user(cmd_init, "Не могу найти этот город, уверен что название правильное?\n");
		return;
	}

	std::string	city;
	std::string	country;
	std::string	temperature;
	std::string	temperature_feels_like;
	std::string	weather;

	size_t i = text.find("name") + 6;
	while (text[i] != ',') {
		city.push_back(text[i]);
		i++;
	}

	i = text.find("country") + 9;
	while (text[i] != ',') {
		country.push_back(text[i]);
		i++;
	}

	i = text.find("temp_c") + 8;
	while (text[i] != ',') {
		temperature.push_back(text[i]);
		i++;
	}

	i = text.find("feelslike_c") + 13;
	while (text[i] != ',') {
		temperature_feels_like.push_back(text[i]);
		i++;
	}

	i = text.find("text") + 6;
	while (text[i] != ',') {
		weather.push_back(text[i]);
		i++;
	}

	server->send_string_to_user(cmd_init, header);
	server->send_string_to_user(cmd_init, "Город: " + city + "\n");

	server->send_string_to_user(cmd_init, header);
	server->send_string_to_user(cmd_init,"Страна: " + country + "\n");

	server->send_string_to_user(cmd_init, header);
	server->send_string_to_user(cmd_init,"Температура: " + temperature + "\n");

	server->send_string_to_user(cmd_init, header);
	server->send_string_to_user(cmd_init,"Ощущается как: " + temperature_feels_like + "\n");

	server->send_string_to_user(cmd_init, header);
	server->send_string_to_user(cmd_init,"Погода: " + weather + "\n");
}
