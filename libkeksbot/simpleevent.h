#ifndef SIMPLEEVENT_H
#define SIMPLEEVENT_H

#include "eventinterface.h"
#include <map>
#include <string>
#include <vector>

class SimpleEvent : public EventHandler
{
private:
	const std::string answerString;
public:
	SimpleEvent(const std::string& answer);
	void OnEvent(Server& srv,
	             const std::string& event,
	             const std::string& origin,
	             const std::vector<std::string>& params);

	EventType GetType()
	{
		return TYPE_SIMPLE;
	}
};

#endif
