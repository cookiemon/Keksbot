#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <libircclient/libircclient.h>

class RestartException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Restarting";
	}
};

class ExitException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Exiting";
	}
};

class IrcException : public std::exception
{
private:
	const int errNum;
public:
	IrcException(int errNum) : errNum(errNum)
	{
	}

	int ErrorNumber() const throw()
	{
		return errNum;
	}

	const char* what() const throw()
	{
		return irc_strerror(errNum);
	}
};

class ConfigException : public std::runtime_error
{
public:
	ConfigException(const std::string& msg)
		: std::runtime_error(msg)
	{
	}
};

#endif
