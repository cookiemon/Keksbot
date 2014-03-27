#ifndef SERVERINTERFACE_H
#define SERVERINTERFACE_H

#include "channel.h"
#include "user.h"
#include <string>

class ServerInterface
{
public:
	virtual ~ServerInterface() { }
	virtual bool IsConnected() = 0;
	virtual void Join(const std::string& chan, const std::string& pw = "") = 0;
	virtual void SendMsg(const std::string& chan, const std::string& msg) = 0;
	virtual void SendAction(const std::string& chan, const std::string& msg) = 0;

	virtual std::string GetName() = 0;
	virtual std::string GetNick() = 0;
	virtual std::string GetLocation() = 0;
	virtual std::string GetUsername() = 0;
	virtual std::string GetRealname() = 0;
	virtual std::string GetPrefix() = 0;

	virtual const Channel& GetChannel(const std::string& chan) = 0;
};

#endif
