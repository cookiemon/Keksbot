#include "unicode.h"
#include "configs.h"
#include "server.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <locale>
#include <stdint.h>

Unicode::Unicode(const Configs& cfg)
{
	cfg.GetValue("dbfile", dbfile);
}

Unicode::~Unicode()
{
}

void Unicode::OnEvent(Server& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
	std::string unicode = params[1];
	std::string codepoint;
	if(unicode.compare(0, 2, "U+") == 0 || unicode.compare(0, 2, "u+") == 0)
	{
		codepoint = std::string(6 - unicode.size(), '0') + unicode.substr(2);
	}
	else
	{
		uint32_t cp;
		size_t numBytes = 1;
		unsigned char mask = 0x7F;
		if(unicode[0] & 0x80)
			while((unicode[0] << numBytes) & 0x80)
			{
				numBytes += 1;
				mask >>= 1;
			}

		if(unicode.size() == numBytes)
		{
			cp = unicode[0] & mask;
			bool failed = false;
			for(size_t i = 1; i < numBytes; ++i)
			{
				if((unicode[i] & 0xC0) != 0x80)
					failed = true;
				cp <<= 6;
				cp |= unicode[i] & 0x3F;
			}
			if(!failed)
			{
				std::stringstream conv;
				conv << std::uppercase << std::hex
					<< std::setfill('0') << std::right << std::setw(4) << cp;
				conv >> codepoint;
			}
		}
	}
	if(codepoint.size() == 0)
	{
		srv.SendMsg(params[0], params[1] + " is not a valid code point or utf-8 character");
		return;
	}

	std::ifstream cpfile;
	cpfile.open(dbfile.c_str());
	std::string cpline;
	while(std::getline(cpfile, cpline))
	{
		if(cpline.compare(0, codepoint.size(), codepoint) == 0)
			break;
	}
	if(cpline.compare(0, codepoint.size(), codepoint) == 0)
		PrintCodePoint(cpline, srv, params[0]);
	else
		srv.SendMsg(params[0], "U+" + codepoint + " does not exist");
}

uint32_t ToUInt32(std::string codepoint)
{
	std::stringstream conv;
	conv << std::hex << codepoint;
	uint32_t cp;
	conv >> cp;
	return cp;
}

std::string ToUtf8(uint32_t cp)
{
	if(cp > 0x1FFFFF)
		return std::string();
	if(cp < 128)
		return std::string(1, static_cast<char>(cp));

	uint8_t bytes = cp > 0x07FF ? cp > 0xFFFF ? 4 : 3 : 2;
	unsigned char mask = ((1 << bytes) - 1) << (8 - bytes);
	std::string str(bytes, '\0');
	str[0] = mask | (cp >> ((bytes - 1) * 6));
	for(uint8_t i = 1; i < bytes; ++i)
	{
		str[i] = 0x80 | ((cp >> ((bytes - 1 - i) * 6)) & 0x3F);
	}
	return str;
}

std::string ToUtf8Text(uint32_t cp)
{
	std::string utf8 = ToUtf8(cp);
	cp = 0;
	for(std::string::iterator it = utf8.begin();
		it != utf8.end();
		++it)
		cp = (cp<<8) | static_cast<unsigned char>(*it);
	std::stringstream conv;
	conv << "0x" << std::uppercase
		<< std::right << std::setfill('0') << std::setw(utf8.size() * 2)
		<< std::hex << cp;
	return conv.str();
}

void Unicode::PrintCodePoint(const std::string& cpline,
	Server& srv,
	const std::string& origin)
{
	size_t start = cpline.find('\t', 0);
	size_t end = cpline.find('\n', start);
	uint32_t cp = ToUInt32(cpline.substr(0, start));
	srv.SendMsg(origin, "Character: " + ToUtf8(cp));
	srv.SendMsg(origin, "Code Point: U+" + cpline.substr(0, start));
	srv.SendMsg(origin, "Name: " + cpline.substr(start+1, (end - start)));
	srv.SendMsg(origin, "UTF-8: " + ToUtf8Text(cp));
}

