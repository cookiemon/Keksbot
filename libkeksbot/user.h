#ifndef USER_H
#define USER_H

#include <string>

struct User
{
	std::string nick;

	User()
	{
	}

	User(const std::string& usrNick)
		: nick(usrNick)
	{
		if(!nick.empty() &&
			(nick[0] == '@' || nick[0] == '+'))
			nick = nick.substr(1);
	}
};

inline bool operator==(const User& left, const User& right)
{
	return left.nick == right.nick;
}

class UserLessCompare
{
public:
	bool operator()(const User& left, const User& right)
	{
		return left.nick < right.nick;
	}
};

#endif
