#ifndef SIMPLEEVENT_H
#define SIMPLEEVENT_H

#include "eventinterface.h"

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

private:
	void LoadAnswers(const std::string& name, std::vector<std::string>& out);
	std::string GetRandomString(std::vector<std::string>& list);
	bool a_to_an(std::string& answer, const size_t start, const size_t end, const std::string& str) const;
	bool is_vowel_sound_exceptions(const std::string& str) const;
};

#endif
