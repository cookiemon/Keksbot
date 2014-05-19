#ifndef HTTPRELAY_H
#define HTTPRELAY_H

#include "configs.h"
#include "eventinterface.h"
#include "libhandle.h"
#include "selectinginterface.h"
#include <curl/curl.h>
#include <map>
#include <set>
#include <string>
#include <vector>

class HttpRelay : public EventHandler, public SelectingInterface
{
private:
	std::string cbUrl;
	std::string server;
	std::set<std::string> handledEvents;
	CURLM* multiHandle;
	LibCurlHandle libcurlhandle;
public:
	HttpRelay(const Configs& cfg);
	~HttpRelay();
	void OnEvent(Server& srv,
		const std::string& event,
		const std::string& origin,
		const std::vector<std::string>& params);
	bool DoesHandle(Server& srv,
		const std::string& event,
		const std::string& origin,
		const std::vector<std::string>& params);

	EventType GetType() { return TYPE_MISC; }

	void AddSelectDescriptors(fd_set& inSet,
		fd_set& outSet,
		fd_set& excSet,
		int& maxFD);
	void SelectDescriptors(fd_set& inSet, fd_set& outSet, fd_set& excSet);

private:
	std::string GetUrlParamList(CURL* handle,
		const std::map<std::string, std::string>& params);
};

#endif
