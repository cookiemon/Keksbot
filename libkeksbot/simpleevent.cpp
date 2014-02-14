#include "simpleevent.h"
#include "server.h"
#include <fstream>
#include <map>
#include <stdlib.h>

typedef std::map<std::string, std::vector<std::string> > AnswerMap;

SimpleEvent::SimpleEvent(EventFilter* filter, const std::string& answer)
	: filter(filter),
	answerString(answer)
{
}

SimpleEvent::~SimpleEvent()
{
	if(filter != NULL)
		delete filter;
}

void SimpleEvent::OnEvent(Server& srv,
                          const std::string& event,
                          const std::string& origin,
                          const std::vector<std::string>& params)
{
	std::string answer = answerString;
	AnswerMap choices;
	choices.insert(AnswerMap::value_type("USER", std::vector<std::string>(1, origin)));
	size_t nextVariable = 0;
	while((nextVariable = answer.find("${", nextVariable)) != std::string::npos)
	{
		nextVariable += 2;
		size_t variableEnd = answer.find('}', nextVariable);
		if(variableEnd != std::string::npos)
		{
			std::string variableName = answer.substr(nextVariable,
			                                        variableEnd - nextVariable);
			AnswerMap::iterator it = choices.find(variableName);
			if(it == choices.end())
			{
				std::vector<std::string> loadedAnswers;
				LoadAnswers(variableName, loadedAnswers);
				std::pair<AnswerMap::iterator, bool> inserted
				                                = choices.insert(AnswerMap::value_type(variableName,
				                                                 loadedAnswers));
				it = inserted.first;
			}
			answer.replace(nextVariable - 2,
			               variableEnd - nextVariable + 3,
						   GetRandomString(it->second)); 
		}
	}
	if(params[0][0] == '#')
		srv.SendMsg(params[0], answer);
	else
		srv.SendMsg(origin, answer);
}

bool SimpleEvent::DoesHandle(Server& srv,
                          const std::string& event,
                          const std::string& origin,
                          const std::vector<std::string>& params)
{
	if(filter != NULL)
		return filter->DoesHandle(srv, event, origin, params);
	else
		return true;
}

void SimpleEvent::LoadAnswers(const std::string& name, std::vector<std::string>& out)
{
	std::ifstream file((name + ".txt").c_str());
	std::string line;
	while(std::getline(file, line))
		out.push_back(line);
	if(out.size() == 0)
		out.push_back("${" + name + "}");
}

const std::string& SimpleEvent::GetRandomString(const std::vector<std::string>& list)
{
	int randNr = rand() % list.size();
	return list[randNr];
}
