#ifndef QUIZZER_H
#define QUIZZER_H

#include "configs.h"
#include "eventinterface.h"
#include "libhandle.h"
#include "selectinginterface.h"
#include <curl/curl.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <regex>

class Quizzer : public EventHandler
{
private:
	std::string searchUrl;
	std::string quizBot;
	std::regex questionRegex;
	CURLM* multiHandle;
	LibCurlHandle libcurlhandle;

	std::vector<std::pair<std::string, size_t> > lastCounts;
	std::vector<std::string> answerQueue;

public:
	Quizzer(const Configs& cfg);
	~Quizzer();
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
};

#endif
