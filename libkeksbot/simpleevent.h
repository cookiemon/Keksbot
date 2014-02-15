#ifndef SIMPLEEVENT_H
#define SIMPLEEVENT_H

#include "eventinterface.h"

class ServerInterface;

class SimpleEvent : public EventHandler
{
private:
	EventFilter* filter;
	const std::string answerString;
public:
	SimpleEvent(EventFilter* filter, const std::string& answer);
	~SimpleEvent();
	void OnEvent(ServerInterface& srv,
	             const std::string& event,
	             const std::string& origin,
	             const std::vector<std::string>& params);

private:
	void LoadAnswers(const std::string& name, std::vector<std::string>& out);
	const std::string& GetRandomString(const std::vector<std::string>& list);
};

#endif
