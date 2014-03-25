#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>
#include <string>
#include <time.h>
#include "user.h"

class Channel;

typedef std::map<std::string, Channel> ChannelListType;
typedef std::set<User, UserLessCompare> UserListType;

struct LastMessage
{
	User usr;
	std::string msg;
	time_t date;
};

struct Channel
{
	UserListType users;
	std::string topic;
	LastMessage last;
};

#endif
