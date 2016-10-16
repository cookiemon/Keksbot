#ifndef NICK_H
#define NICK_H

#include <ostream>
#include <iostream>

class Nick
{
private:
	std::string name;
public:
	explicit Nick(const std::string& n) : name(n) { /* empty */ }

friend std::ostream& operator<< (std::ostream& out, const Nick& nick)
{
std::cout << "Test: \"" << nick.name << "\"\n";
	const char* name = nick.name.c_str();
	out << *name << "\xEF\xBB\xBF" << (name+1);
	return out;
}
};

#endif

