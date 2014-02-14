#include "simpleevent.h"
#include "server.h"
#include <fstream>
#include <map>
#include <stdlib.h>

typedef std::map<std::string, std::vector<std::string> > AnswerMap;
typedef std::vector<std::string> StringList;

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
	choices.insert(AnswerMap::value_type("USER", StringList(1, origin)));
	if(params.size() > 1)
		choices.insert(AnswerMap::value_type("CHAN", StringList(1, params[0])));
	const std::string& msg = params[params.size() - 1];
	size_t msgStart = msg.find_first_of(" \t\n\r");
	msgStart = msg.find_first_not_of(" \r\t\n", msgStart);
	if(msgStart != std::string::npos)
		choices.insert(AnswerMap::value_type("MSG", StringList(1, msg.substr(msgStart))));

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
				StringList loadedAnswers;
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
