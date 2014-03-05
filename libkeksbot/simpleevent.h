#ifndef SIMPLEEVENT_H
#define SIMPLEEVENT_H

#include "eventinterface.h"

class ServerInterface;

class SimpleEvent : public EventHandler
{
private:
	const std::string answerString;
public:
	SimpleEvent(const std::string& answer);
	void OnEvent(ServerInterface& srv,
	             const std::string& event,
	             const std::string& origin,
	             const std::vector<std::string>& params);
	
	EventType GetType()
	{
		return TYPE_SIMPLE;
	}

private:
	void LoadAnswers(const std::string& name, std::vector<std::string>& out);
	std::string GetRandomString(std::vector<std::string>& list);
};

#endif
