#include "udsserver.h"
#include "configs.h"
#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
#include "serverinterface.h"
#include "stringhelpers.h"
#include <algorithm>
#include <errno.h>
#include <unistd.h>

UdsServer::UdsServer(EventManager* man, const Configs& settings)
	: man(man)
{
	if(man == NULL)
		throw IllegalArgumentException("EventManager* may not be null (UdsServer::UdsServer)");

	std::string boundServer;
	settings.GetValue("server", boundServer);
	srv = man->GetServer(boundServer);

	std::string address;
	settings.GetValue("location", address);
	if(address.size() > UNIX_PATH_MAX - 1)
		throw ConfigException("Location for UdsServer too long");

	srvSock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if(srvSock == -1)
		throw SystemException(errno);

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, address.c_str(), UNIX_PATH_MAX);
	addr.sun_path[UNIX_PATH_MAX - 1] = '\0';
	int res = bind(srvSock,
		reinterpret_cast<struct sockaddr*>(&addr),
		sizeof(sockaddr_un));
	if(res == -1)
	{
		int err = errno;
		unlink(addr.sun_path);
		throw SystemException(err);
	}
	res = listen(srvSock, 50);
	if(res == -1)
	{
		int err = errno;
		unlink(addr.sun_path);
		throw SystemException(err);
	}
}

UdsServer::~UdsServer()
{
	for(std::vector<int>::const_iterator it = clients.begin();
		it != clients.end();
		++it)
		close(*it);
	close(srvSock);
	unlink(addr.sun_path);
}

void UdsServer::AddSelectDescriptors(fd_set& inFD, fd_set& outFD, int& maxFD)
{
	FD_SET(srvSock, &inFD);
	maxFD = std::max(srvSock, maxFD);
	for(std::vector<int>::const_iterator it = clients.begin();
		it != clients.end();
		++it)
	{
		FD_SET(*it, &inFD);
		maxFD = std::max(*it, maxFD);
	}
}

void UdsServer::SelectDescriptors(fd_set& inFD, fd_set& outFD)
{
	int res = 0;
	if(FD_ISSET(srvSock, &inFD))
	{
		while((res = accept(srvSock, NULL, NULL)) != -1)
			clients.push_back(res);
		if(errno != EWOULDBLOCK)
			throw SystemException(errno);
	}
	
	for(std::vector<int>::iterator it = clients.begin();
		it != clients.end();
		++it)
	{
		if(FD_ISSET(*it, &inFD))
		{
			char buf[1024];
			ssize_t numRead = recv(*it, buf, 1024, MSG_DONTWAIT);
			buf[numRead] = '\0';
			if(numRead > 0)
			{
				printf("%s", buf);
				ParseMessage(*it, buf);
				close(*it);
			}
			else
			{
				std::vector<int>::iterator back = clients.end();
				--back;
				std::swap(*it, *back);
				clients.pop_back();
				--it;
			}
		}
	}
}

void UdsServer::ParseMessage(int fd, std::string msg)
{
	Log(LOG_DEBUG, "Received UDS Message: %s", msg.c_str());

	Trim(msg);

	std::string cmd = CutFirstWord(msg);

	if(cmd == "send")
	{
		std::string target = CutFirstWord(msg);
		srv->SendMsg(target, msg);
	}
	else if(cmd == "get")
	{
		std::string what = CutFirstWord(msg);
		if(what == "userlist")
		{
			const UserListType& usrList = srv->GetChannel(msg).users;
			std::string reply = "[";
			reply.reserve(128);

			UserListType::const_iterator it = usrList.begin();
			if(it != usrList.end())
			{
				reply += "\"" + it->nick + "\"";
				++it;
			}

			for(;
				it != usrList.end();
				++it)
				reply += ", \"" + it->nick + "\"";
			reply += "]";

			SendReply(fd, reply);

		}
		else if(what == "usercount")
		{
			const Channel& chan = srv->GetChannel(msg);
			size_t num = chan.users.size();
			std::stringstream sstr;
			sstr << num;
			SendReply(fd, sstr.str());
		}
		else if(what == "topic")
		{
			const Channel& chan = srv->GetChannel(msg);
			SendReply(fd, chan.topic);
		}
		else if(what == "lastmessage")
		{
			const Channel& chan = srv->GetChannel(msg);
			struct tm timeStruct;
			if(localtime_r(&chan.last.date, &timeStruct) == NULL)
				throw SystemException(errno);
			char ftime[26];
			size_t len = strftime(ftime, 25, "%FT%T%z", &timeStruct);
			if(len == 0)
				throw SystemException(errno);
			ftime[24] = ftime[23];
			ftime[23] = ftime[22];
			ftime[22] = ':';
			std::string rply = "{\"channel\":\"" + EscapeString(msg, "\\\"", "\\")
				+ "\",\"user\":\"" + EscapeString(chan.last.usr.nick, "\\\"", "\\")
				+ "\",\"time\":\"" + ftime
				+ "\",\"message\":\"" + EscapeString(chan.last.msg, "\\\"", "\\")
				+ "\"}";
			SendReply(fd, rply);
		}
	}
}

void UdsServer::SendReply(int fd, const std::string& reply)
{
	size_t num = write(fd, reply.c_str(), reply.size());
	if(num < reply.size())
		throw SystemException(errno);
}

EventType UdsServer::GetType()
{
	return TYPE_MISC;
}

void UdsServer::OnEvent(ServerInterface& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
}

