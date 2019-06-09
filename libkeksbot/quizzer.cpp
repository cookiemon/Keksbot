#include "quizzer.h"
#include "configs.h"
#include "eventmanager.h"
#include "logging.h"
#include "server.h"
#include <algorithm>
#include <curl/curl.h>
#include <random>

Quizzer::Quizzer(const Configs& cfg)
{
	multiHandle = curl_multi_init();

	std::string regexString;
	cfg.GetValue("question", regexString);
	questionRegex = std::regex(regexString);
	cfg.GetValue("search", searchUrl);
	cfg.GetValue("bot", quizBot);

	Log(LOG_INFO, "Quizzer initialized with regex %s", regexString.c_str());
}

Quizzer::~Quizzer()
{
	curl_multi_cleanup(multiHandle);
}

void Quizzer::OnEvent(Server& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
	if(params.empty())
		return;
	const auto &message = params.back();

	bool matches = std::regex_match(message, questionRegex);
	if(matches) return;

	auto rng = std::uniform_int_distribution<>(0, 3);
	auto gen = std::mt19937{std::random_device()()};
	auto num = rng(gen);

	char answer[2] = "a";
	answer[0] += num;

	srv.SendMsg(params[0], answer);
}

bool Quizzer::DoesHandle(Server& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
	return event == "CHANNEL" && origin == quizBot;
}

