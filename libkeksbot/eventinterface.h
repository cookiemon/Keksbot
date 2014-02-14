#ifndef EVENTINTERFACE_H
#define EVENTINTERFACE_H

#include "configs.h"
#include <string>
#include <vector>

class Server;

class EventFilter
{
public:
	virtual ~EventFilter() { }
	virtual bool DoesHandle(Server& srv,
	                        const std::string& event,
        	                const std::string& origin,
	                        const std::vector<std::string>& params) = 0;
};

class EventHandler
{
private:
	std::string description;
public:
	virtual ~EventHandler() { }
	virtual void OnEvent(Server& srv,
	                     const std::string& event,
        	             const std::string& origin,
	                     const std::vector<std::string>& params) = 0;
	virtual bool DoesHandle(Server& srv,
	                        const std::string& event,
	                        const std::string& origin,
	                        const std::vector<std::string>& params) = 0;

	const std::string& GetDescription()
	{
		return description;
	}
	void SetDescription(const std::string& newDesc)
	{
		description = newDesc;
	}
};

#endif
