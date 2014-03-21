#include <fcgi_stdio.h>
#include "udsclient.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <vector>

int dehex(char hex)
{
	if('0' <= hex && hex <= '9')
		return hex - '0';
	else if('a' <= hex && hex <= 'f')
		return hex - 'a';
	else if('A' <= hex && hex <= 'F')
		return hex - 'A';
	else
		return -1;
}

int dehex(const char* hex, int digits)
{
	int unhexed = 0;
	for(int i = 0; i < digits; ++i)
	{
		int nextDig = dehex(hex[i]);
		if(nextDig < 0)
			return -1;

		unhexed <<= 4;
		unhexed |= nextDig;
	}
	return unhexed;
}

bool urldecode(char* str, char** end)
{
	const char* it = str;

	while(it != *end)
	{
		if(*it == '+')
		{
			*str = ' ';
			++it;
			++str;
		}
		else if(*it == '%')
		{
			int ch = dehex(it+1, 2);
			if(ch < 0)
				return true;

			*str = static_cast<char>(ch);

			++str;
			it += 3;
		}
		else
		{
			*str = *it;
			++it;
			++str;
		}
	}
	*str = '\0';
	*end = str;

	return false;
}

std::map<std::string, std::string> getPost()
{
	char* tmp = getenv("CONTENT_LENGTH");
	size_t length = atoi(tmp);

	std::vector<char> buf(length + 1, '\0');
	fread(&buf[0], 1, length, stdin);

	std::map<std::string, std::string> retVal;

	char* begin = &buf[0];
	char* end = begin + length;
	while(begin != end)
	{
		char* endOfVal = std::find(begin, end, '\n');
		char* endOfVar = std::find(begin, endOfVal, '=');
		char* begOfVar = begin;
		char* begOfVal = endOfVar;
		if(begOfVal < endOfVal)
			begOfVal += 1;
		begin = endOfVal!=end ? endOfVal + 1 : end;

		urldecode(begOfVar, &endOfVar);
		urldecode(begOfVal, &endOfVal);

		retVal.insert(std::pair<std::string, std::string>(
				std::string(begOfVar, endOfVar),
				std::string(begOfVal, endOfVal)));
	}

	return retVal;
}

std::vector<std::string> getUrl()
{
	char* url = getenv("SCRIPT_NAME");
	std::vector<std::string> retVal;

	if(url != NULL)
	{
		if(*url == '/')
			url += 1;
		char* end = url + strlen(url);
		while(url != end)
		{
			char* strBegin = url;
			char* strEnd = std::find(url, end, '/');
			url = strEnd!=end ? strEnd+1 : end;

			urldecode(strBegin, &strEnd);
			retVal.push_back(std::string(strBegin, strEnd));
		}
	}

	return retVal;
}

int main(void)
{
	while(FCGI_Accept() >= 0)
	{
		UdsClient cl = UdsClient("/home/genion/Keksbot/keksbot.sock");
		const char* method = getenv("REQUEST_METHOD");
		if(method == NULL)
			method = "GET";

		std::vector<std::string> args = getUrl();

		if(args[0] == "async")
			args.erase(args.begin());
		if(args.size() < 3)
			continue;

		if(method == std::string("POST") && args[2] == std::string("messages"))
		{
			if(args[0] == "channel")
				args[1] = "#" + args[1];
			else if(args[0] != "user")
				continue;

			std::map<std::string, std::string> params = getPost();
			std::string text = params["text"];
			if(text.empty())
				continue;

			cl.Send("send " + args[1] + " " + text);
		}
		else if(args[0] == std::string("channel") && args.size() > 2)
		{
			if(args[2] == "users")
			{
				cl.Send(std::string("get userlist #") + args[1]);
				std::string str = cl.Read();
				printf("%s", str.c_str());
			}
		}
	}

	return 0;
}
