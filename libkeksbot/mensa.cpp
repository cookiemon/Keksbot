#include "mensa.h"

#include "logging.h"
#include "server.h"
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <time.h>

void RoundToDay(struct tm* time)
{
	assert(time != NULL);
	time->tm_sec = 0;
	time->tm_min = 0;
	time->tm_hour = 0;
}

static const char* const weekday[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
void WriteTime(std::ostream& out, time_t time)
{
	struct tm localnow;
	localtime_r(&time, &localnow);
	out << weekday[localnow.tm_wday] << ". "
		<< localnow.tm_mday << "." << (localnow.tm_mon + 1)
		<< "." << (localnow.tm_year % 100);
}

Mensa::Mensa(const Configs& cfg)
	: multiHandle(curl_multi_init()),
	lastupdate(0),
	updating(false)
{
	cfg.GetValue("menuurl", menuurl);
	cfg.GetValue("metaurl", metaurl);
	cfg.GetValue("login", login);
	cfg.GetValue("canteen", canteen);
	std::string rawlines;
	cfg.GetValue("lines", rawlines);
	std::istringstream sstr(rawlines);
	std::string newline;
	while(std::getline(sstr, newline, ','))
		lines.insert(newline);
}

Mensa::~Mensa()
{
	curl_multi_cleanup(multiHandle);
}

void Mensa::OnEvent(Server& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
	if(params.size() == 0)
		throw std::logic_error("OnEvent should only be called for SENDMSG");
	int offset = 0;
	if(params.size() > 1 && !params[1].empty())
	{
		std::stringstream sstr(params[1]);
		sstr >> offset;
		if(sstr.fail())
		{
			srv.SendMsg(params[0], "Error: " + params[1] + " is not a number");
			return;
		}
	}
	time_t now = time(NULL);
	// No error handling, I don't care about overflow in time_t
	if(difftime(now, lastupdate) > (3600. * 24.))
	{
		originBuf.insert(QueuedResponse(&srv, params[0], offset));
		if(!updating)
			QueryMenuUpdate();
	}
	else
		SendMenu(srv, params[0], offset);
}

void Mensa::AddSelectDescriptors(fd_set& inSet,
	fd_set& outSet,
	fd_set& excSet,
	int& maxFD)
{
	int max = 0;
	curl_multi_fdset(multiHandle, &inSet, &outSet, &excSet, &max);
	maxFD = std::max(maxFD, max);
}

bool Mensa::UpdateMeta(rapidjson::Value& val)
{
	if(!val.IsObject() || val.IsNull())
		return true;

	rapidjson::Value::ConstMemberIterator canteenjs = val.FindMember(canteen.c_str());
	if(canteenjs == val.MemberEnd()
		|| !canteenjs->value.IsObject()
		|| canteenjs->value.IsNull())
		return true;
	
	rapidjson::Value::ConstMemberIterator linesjs=canteenjs->value.FindMember("lines");
	if(linesjs == canteenjs->value.MemberEnd()
		|| !linesjs->value.IsObject()
		|| linesjs->value.IsNull())
		return true;

	for(rapidjson::Value::ConstMemberIterator i = linesjs->value.MemberBegin();
		i != linesjs->value.MemberEnd(); ++i)
	{
		if(!i->value.IsString() || !i->name.IsString())
			continue;
		lineMap[i->name.GetString()] = i->value.GetString();
		Log(LOG_DEBUG, "Updated line name for %s(%s)",
			i->name.GetString(), i->value.GetString());
	}
	return false;
}

bool Mensa::UpdateMenu(rapidjson::Value& val)
{
	if(!val.IsObject() || val.IsNull())
		return true;
	menu = val;
	return false;
}

void Mensa::SelectDescriptors(fd_set& inSet, fd_set& outSet, fd_set& excSet)
{
	int running;
	curl_multi_perform(multiHandle, &running);
	struct CURLMsg* msg;
	int msgnum;
	bool hasfinished = false;
	while((msg = curl_multi_info_read(multiHandle, &msgnum)))
	{
		hasfinished = true;
		CURL* handle = msg->easy_handle;
		Log(LOG_DEBUG, "Result of curl request: %ld", static_cast<long>(msg->data.result));
		curl_multi_remove_handle(multiHandle, handle);
		curl_easy_cleanup(handle);
	}

	if(hasfinished && !running)
	{
		bool err = false;
		doc.Parse(metabuf.c_str());
		if(doc.IsObject() && doc.HasMember("mensa"))
			err = UpdateMeta(doc["mensa"]);
		doc.Parse(menubuf.c_str());
		if(doc.IsObject() && doc.HasMember(canteen.c_str()))
			err = err || UpdateMenu(doc[canteen.c_str()]);

		if(err)
		{
			for(std::set<QueuedResponse>::iterator it = originBuf.begin();
				it != originBuf.end();
				++it)
				it->srv->SendMsg(it->channel, "Error while receiving json");
		}
		else
		{
			for(std::set<QueuedResponse>::iterator it = originBuf.begin();
				it != originBuf.end();
				++it)
				SendMenu(*it->srv, it->channel, it->offset);
		}
		originBuf.clear();
		time_t now = time(NULL);
		struct tm tm;
		localtime_r(&now, &tm);
		RoundToDay(&tm);
		now = mktime(&tm);
		lastupdate = now;
		updating = false;
	}
}

void Mensa::RegisterCurlHandle(const std::string& url, std::string& buffer)
{
	buffer = std::string();
	CURL* handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &Mensa::PushData);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "$USERAGENTWITHMOZILLAGECKOSAFARIANDSHIT");

	curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(handle, CURLOPT_USERPWD, login.c_str());

	curl_multi_add_handle(multiHandle, handle);

	Log(LOG_DEBUG, "Requested url %s", url.c_str());
}

void Mensa::QueryMenuUpdate()
{
	updating = true;

	RegisterCurlHandle(menuurl, menubuf);
	RegisterCurlHandle(metaurl, metabuf);
}

size_t Mensa::PushData(char* data, size_t size, size_t nmemb, void* userdata)
{
	std::string* buffer = static_cast<std::string*>(userdata);
	if(buffer != NULL)
		buffer->append(data, size*nmemb);
	return nmemb;
}

void Mensa::SendMenu(Server& srv, const std::string& channel, int offset)
{
	time_t now = time(NULL);
	struct tm localnow;
	localtime_r(&now, &localnow);
	RoundToDay(&localnow);
	if(offset)
		localnow.tm_mday += offset;
	else if(localnow.tm_wday % 6 == 0)
		localnow.tm_mday += localnow.tm_wday?2:1;

	now = mktime(&localnow);
	std::stringstream sstr;
	sstr << now;
	std::string strnow(sstr.str());

	if(!menu.HasMember(strnow.c_str()))
	{
		std::stringstream errmsg;
		errmsg << "Keine Daten vorhanden für ";
		WriteTime(errmsg, now);
		srv.SendMsg(channel, errmsg.str());
		return;
	}

	rapidjson::Value& menunow = menu[strnow.c_str()];
	if(!menunow.IsObject() || menunow.IsNull())
		return;

	std::stringstream menumsg;
	menumsg << "Mensaeinheitsbrei am ";
	WriteTime(menumsg, now);
	srv.SendMsg(channel, menumsg.str());

	for (rapidjson::Value::ConstMemberIterator itr = menunow.MemberBegin();
	    itr != menunow.MemberEnd(); ++itr)
	{
		if(!itr->name.IsString())
			continue;

		std::string name = itr->name.GetString();
		if(lines.count(name) != 0)
			SendLine(srv, channel, name, itr->value);
		else
			SendLineClosed(srv, channel, name, itr->value);
	}
}

void Mensa::SendLineClosed(Server& srv, const std::string& origin,
	const std::string& line, const rapidjson::Value& value)
{
	if(!value.IsArray() || value.Size() != 1)
		return;

	const rapidjson::Value& data = value[0];
	if(!data.IsObject() || data.IsNull())
		return;

	rapidjson::Value::ConstMemberIterator itr = data.FindMember("closing_end");
	if(itr == data.MemberEnd() || !itr->value.IsInt64())
		return;

	std::stringstream sstr;
	std::string linename = lineMap[line];
	if(linename.empty())
		linename = line;
	sstr << linename << ": Geschlossen bis ";
	WriteTime(sstr, itr->value.GetInt64());

	itr = data.FindMember("closing_text");
	if(itr != data.MemberEnd() && itr->value.IsString())
		sstr << " (" << itr->value.GetString() << ")";

	srv.SendMsg(origin, sstr.str());
}

void Mensa::SendLine(Server& srv, const std::string& origin,
	const std::string& line, const rapidjson::Value& value)
{
	if(!value.IsArray())
		return;
	if(value.Size() == 0)
		return;

	std::string linename = lineMap[line];
	if(linename.empty())
		linename = line;

	/* Check first array element for special values */
	const rapidjson::Value& firstElem = value[0];
	if(firstElem.IsObject() && !firstElem.IsNull())
	{
		if(firstElem.HasMember("closing_end"))
		{
			SendLineClosed(srv, origin, line, value);
			return;
		}
		rapidjson::Value::ConstMemberIterator itr = firstElem.FindMember("nodata");
		if(itr != firstElem.MemberEnd()
			&& itr->value.IsBool() && itr->value.GetBool())
		{
			srv.SendMsg(origin, linename + ": Keine Daten vorhanden");
			return;
		}
	}

	std::stringstream strstr;
	strstr.setf(std::ios_base::fixed);
	strstr.precision(2);
	strstr << linename << ": ";
	for(rapidjson::Value::ConstValueIterator it = value.Begin();
		it != value.End();
		++it)
	{
		if(!it->IsObject() || it->IsNull())
			continue;

		rapidjson::Value::ConstMemberIterator meal = it->FindMember("meal");
		rapidjson::Value::ConstMemberIterator price_1 = it->FindMember("price_1");
		if(meal == it->MemberEnd() || !meal->value.IsString()
			|| price_1 == it->MemberEnd() || !price_1->value.IsNumber())
			continue;

		double price = price_1->value.GetDouble();
		// threshold
		if(price < 1.3)
			continue;

		strstr << meal->value.GetString();
		rapidjson::Value::ConstMemberIterator dish = it->FindMember("dish");
		if(dish != it->MemberEnd()
			&& dish->value.IsString() && dish->value.GetStringLength() != 0)
			strstr << " " << dish->value.GetString();

		strstr << " (" << price << "), ";
	}
	std::string str = strstr.str();
	srv.SendMsg(origin, str.substr(0, str.size()-2));
}
