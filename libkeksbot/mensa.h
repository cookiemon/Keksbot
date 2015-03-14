#ifndef MENSA_H
#define MENSA_H

#include "configs.h"
#include "eventinterface.h"
#include "libhandle.h"
#include "selectinginterface.h"
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <utility>

class Mensa : public EventHandler, public SelectingInterface
{
private:
	std::string menuurl;
	std::string metaurl;
	std::string login;
	std::string canteen;
	std::vector<std::string> lines;
	std::map<std::string, std::string> lineMap;
	CURLM* multiHandle;
	LibCurlHandle libcurlhandle;

	rapidjson::Document doc;
	rapidjson::Value menu;
	time_t lastupdate;

	std::string menubuf;
	std::string metabuf;
	std::vector<std::pair<Server*, std::string> > originBuf;
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
	void UpdateMenu(Server& srv, const std::string& channel, const std::vector<std::string>& params);
	void SendMenu(Server& srv, const std::string& channel, const std::vector<std::string>& params);
	static size_t PushData(char* data, size_t size, size_t nmemb, void* userdata);
	void RegisterCurlHandle(const std::string& url, std::string& buffer);
	void SendLine(Server& srv, const std::string& origin, const std::string& line, rapidjson::Value& value);
};

#endif
