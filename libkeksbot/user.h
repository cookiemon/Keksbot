#ifndef USER_H
#define USER_H

#include <string>

struct User
{
	std::string nick;

	User()
	{
	}

	User(const std::string& nick)
		: nick(nick)
	{
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
