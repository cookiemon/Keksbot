#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>
#include <string>
#include "user.h"

class Channel;

typedef std::map<std::string, Channel> ChannelListType;
typedef std::set<User, UserLessCompare> UserListType;

struct Channel
{
	UserListType users;
	std::string topic;
};

#endif
