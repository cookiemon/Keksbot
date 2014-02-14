#ifndef SERVER_H
#define SERVER_H

#include "configs.h"
#include <string>
#include <vector>
#include <libircclient/libircclient.h>

typedef std::vector<std::string> ParamList;

class EventManager;

class Server
{
public:
	typedef std::vector<std::string> ChannelListType;
private:
	irc_callbacks_t callbacks;
	irc_session_t* session;

	std::string name;
	std::string srv;
	unsigned short port;
	std::string passwd;
	std::string nick;
	std::string username;
	std::string realname;
	char prefix;
	ChannelListType channels;

	EventManager* manager;
public:
	Server(const std::string& name, const KeyValueMap& settings);

	~Server(void);

	void Connect(void);
	void AddSelectDescriptors(fd_set& inSet, fd_set& outSet, int& maxFd);
	void SelectDescriptors(fd_set& inSet, fd_set& outSet);
	void EventConnect(const std::string& evt, const std::string& origin, const ParamList& args);
	void EventNumeric(unsigned int       evt, const std::string& origin, const ParamList& args);
	void EventMisc   (const std::string& rvt, const std::string& origin, const ParamList& args);

	bool IsConnected(void);
	void Join(const std::string& chan, const std::string& pw = "");
	void SendMsg(const std::string& chan, const std::string& msg);
	void SendAction(const std::string& chan, const std::string& msg);

	const std::string& GetName(void);
	const std::string& GetLocation(void);
	unsigned short GetPort(void);
	const std::string& GetNick(void);
	const std::string& GetUsername(void);
	const std::string& GetRealname(void);
	char GetPrefix(void);
	const ChannelListType& GetChannels(void);

	void SetManager(EventManager* newManager)
	{
		manager = newManager;
	}

private:
	void Init(void);
	void AddSettingOrDefault(const KeyValueMap& settings,
		std::string& attribute,
		const std::string& key,
		const std::string& deflt);
	void LogIrcEvent(const std::string& evt, const std::string& origin, const ParamList& params);
};

#endif
