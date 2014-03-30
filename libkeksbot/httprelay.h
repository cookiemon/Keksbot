#ifndef HTTPRELAY_H
#define HTTPRELAY_H

#include "configs.h"
#include "eventinterface.h"
#include "libhandle.h"
#include <curl/curl.h>
#include <map>
#include <set>
#include <string>
#include <vector>

class HttpRelay : public EventHandler
{
private:
	std::string cbUrl;
	std::string server;
	std::set<std::string> handledEvents;
	LibCurlHandle libcurlhandle;
public:
	HttpRelay(const Configs& cfg);
	void OnEvent(ServerInterface& srv,
		const std::string& event,
		const std::string& origin,
		const std::vector<std::string>& params);
	bool DoesHandle(ServerInterface& srv,
		const std::string& event,
		const std::string& origin,
		const std::vector<std::string>& params);

	EventType GetType() { return TYPE_MISC; }

private:
	std::string GetUrlParamList(CURL* handle,
		const std::map<std::string, std::string>& params);
};

#endif
