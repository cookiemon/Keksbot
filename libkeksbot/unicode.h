#ifndef UNICODE_H
#define UNICODE_H

#include "configs.h"
#include "eventinterface.h"
#include "selectinginterface.h"
#include <string>
#include <vector>

class Unicode : public EventHandler
{
private:
	std::string dbfile;
public:
	Unicode(const Configs& cfg);
	~Unicode();
	void OnEvent(Server& srv,
		const std::string& event,
		const std::string& origin,
		const std::vector<std::string>& params);

	EventType GetType() { return TYPE_SIMPLE; }

private:
	void PrintCodePoint(const std::string& cpline, Server& srv, const std::string& origin);
};

#endif
