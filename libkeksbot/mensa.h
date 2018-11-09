#ifndef MENSA_H
#define MENSA_H

#include "configs.h"
#include "eventinterface.h"
#include "libhandle.h"
#include "selectinginterface.h"
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <set>
#include <utility>
#include <chrono>

class Mensa : public EventHandler, public SelectingInterface
{
private:
	struct QueuedResponse
	{
		Server* srv;
		std::string origin;
		std::string channel;
		int offset;
		QueuedResponse(Server* newSrv, const std::string &origin, const std::string& newChan, int newOff)
			: srv(newSrv), origin(origin), channel(newChan), offset(newOff) { /* nothing */ }
		bool operator<(const QueuedResponse& other) const
		{ return srv < other.srv || channel < other.channel || offset < other.offset; }
	};
	std::string ad;
	std::string menuurl;
	std::string metaurl;
	std::string login;
	std::string canteen;
	std::set<std::string> lines;
	std::map<std::string, std::string> lineMap;
	CURLM* multiHandle;
	LibCurlHandle libcurlhandle;

	rapidjson::Document doc;
	rapidjson::Value menu;
	std::chrono::steady_clock::time_point lastupdate;

	std::string menubuf;
	std::string metabuf;
	std::set<QueuedResponse> originBuf;
	bool updating;

public:
	Mensa(const Configs& cfg);
	~Mensa();
	void OnEvent(Server& srv,
		const std::string& event,
		const std::string& origin,
		const std::vector<std::string>& params);
	
	EventType GetType() { return TYPE_SIMPLE; }

	void AddSelectDescriptors(fd_set& inSet,
		fd_set& outSet,
		fd_set& excSet,
		int& maxFD);
	void SelectDescriptors(fd_set& inSet, fd_set& outSet, fd_set& excSet);

private:
	bool UpdateMeta(rapidjson::Value& val);
	bool UpdateMenu(rapidjson::Value& val);
	void QueryMenuUpdate();
	void SendMenu(Server& srv, const std::string& origin, const std::string& channel, int offset);
	static size_t PushData(char* data, size_t size, size_t nmemb, void* userdata);
	void RegisterCurlHandle(const std::string& url, std::string& buffer);
	void SendLineClosed(Server& srv, const std::string& origin, const std::string& line,
		const rapidjson::Value& value);
	void SendLine(Server& srv, const std::string& origin, const std::string& line,
		const rapidjson::Value& value);
};

#endif
