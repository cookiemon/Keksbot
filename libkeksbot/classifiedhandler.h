#ifndef CLASSIFIEDHANDLER_H
#define CLASSIFIEDHANDLER_H

#include "eventinterface.h"

#include <ctype.h>
#include <string>
#include <iostream>

struct CaesarTransform
{
	char operator()(char c)
	{
		int ch = (unsigned char)c;
		if(!isalpha(ch))
			return c;

		bool islc = islower(ch);
		
		ch += 13;
		if((islc && ch > 'z') || (!islc && ch > 'Z'))
			ch -= 26;
		return (char)ch;
	}
};

struct Base64Decode
{
	std::string operator()(const std::string& str)
	{
		std::string retVal;
		retVal.reserve(str.size() * 3 / 4);

		int tmp = 0;

		for(size_t i = 0; i < str.size(); ++i)
		{
			int val = -1;
			const char curC = str[i];
			if('A' <= curC && curC <= 'Z')
				val = curC - 'A';
			else if('a' <= curC && curC <= 'z')
				val = curC - 'a' + 26;
			else if('0' <= curC && curC <= '9')
				val = curC - '0' + 26*2;
			else if(curC == '+')
				val = 62;
			else if(curC == '/')
				val = 63;
			else if(curC == '=')
				break;
			else
				return "";

			switch(i%4)
			{
			case 0:
				tmp = val;
				break;
			case 1:
			case 2:
			case 3:
			{
				int decoded = tmp << (i%4)*2;
				decoded |= val >> (6-(i%4)*2);
				decoded &= 0xFF;
				if(!isprint(decoded))
					return "";
				retVal.push_back((char)decoded);
				tmp = val;
				break;
			}
			}
		}
		return retVal;
	}
};

class ClassifiedHandler : public EventHandler
{
public:
	EventType GetType()
	{
		return TYPE_MISC;
	}

	bool DoesHandle(ServerInterface& srv,
				const std::string& event,
				const std::string& origin,
				const std::vector<std::string>& params)
	{
		return event == "CHANNEL" || event == "PRIVMSG";
	}

	void OnEvent(ServerInterface& srv,
				const std::string& event,
				const std::string& origin,
				const std::vector<std::string>& params);
};

#endif
