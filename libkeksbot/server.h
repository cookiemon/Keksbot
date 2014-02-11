#ifndef SERVER_H
#define SERVER_H

#include "configs.h"
#include <string>
#include <vector>
#include <libircclient/libircclient.h>

typedef std::vector<std::string> ParamList;

class Server
{
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
	std::vector<std::string> channels;
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

private:
	void Init(void);
	void AddSettingOrDefault(const KeyValueMap& settings,
		std::string& attribute,
		const std::string& key,
		const std::string& deflt);
};

#endif
