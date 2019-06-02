#ifndef DATE_H
#define DATE_H

#include "configs.h"
#include "eventinterface.h"
#include "selectinginterface.h"
#include <string>
#include <vector>

class Date : public EventHandler
{
private:
	std::string dbfile;
public:
	Date(const Configs& cfg);
	~Date();
	void OnEvent(Server& srv,
		const std::string& event,
		const std::string& origin,
		const std::vector<std::string>& params);

	EventType GetType() { return TYPE_SIMPLE; }
};

#endif
