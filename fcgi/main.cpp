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
		return 10 + hex - 'a';
	else if('A' <= hex && hex <= 'F')
		return 10 + hex - 'A';
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
	size_t length = static_cast<size_t>(atoi(tmp));

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

std::string ParseCmd(const std::string& method,
	const std::vector<std::string>& args)
{
	if(args.empty())
		return "";

	if(method == std::string("POST"))
	{
		if(args.size() > 2 && args[2] == "messages")
		{
			std::map<std::string, std::string> params = getPost();
			std::string text = params["text"];
			if(text.empty())
				return "";

			return "send " + args[1] + " " + text;
		}
		else if(args.size() > 1 && args[0] == "hooks")
		{
			std::map<std::string, std::string> params = getPost();
			std::string url = params["url"];
			if(url.empty())
				return "";
			return "set hook " + args[1] + " " + url;
		}
		else if(args.size() > 3 && args[2] == "hooks")
		{
			std::map<std::string, std::string> params = getPost();
			std::string url = params["url"];
			if(url.empty())
				return "";
			std::string pattern = params["pattern"];
			return "set hook " + args[3] + " " + args[1] + " " + url + " " + pattern;
		}
	}
	else if(method == std::string("GET"))
	{
		if(args.size() > 2 && args[0] == std::string("channel"))
		{
			if(args[2] == "users")
			{
				if(args.size() > 3 && args[3] == "count")
					return "get usercount " + args[1];
				else
					return "get userlist " + args[1];
			}
			else if(args[2] == "topic")
				return "get topic " + args[1];
			else if(args[2] == "stats")
				return "get stats " + args[1];
			else if(args.size() > 3 && args[2] == "messages" && args[3] == "last")
				return "get lastmessage " + args[1];
			else
				return "";
		}
		else if(args[0] == "hooks")
		{
			return "get hooks";
		}
	}
	else if(method == std::string("DELETE"))
	{
		if(args.size() > 1 && args[0] == "hooks")
		{
			return "del hook " + args[1];
		}
	}
	return "";
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

		bool sync = true;
		if(args[0] == "async")
		{
			args.erase(args.begin());
			sync = false;
		}

		if(args.size() > 1 && args[0] == "channel")
			args[1] = "#" + args[1];

		std::string cmd = ParseCmd(method, args);
		if(cmd.empty())
		{
			printf("Status: 404 Ressource not Found\n\n");
			continue;
		}
		cmd += '\n';

		cl.Send(cmd);
		if(sync)
		{
			std::string reply = cl.Read();
			if(!reply.empty())
			{
				if(method == std::string("GET"))
					printf("Access-Control-Allow-Origin: *\n");
				printf("Content-type: text/plain\n\n");
				printf("%s\n", reply.c_str());
			}
		}
	}

	return 0;
}
