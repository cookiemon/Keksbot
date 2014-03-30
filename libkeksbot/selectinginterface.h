#ifndef SELECTINGINTERFACE_H
#define SELECTINGINTERFACE_H

#include <sys/select.h>

class SelectingInterface
{
public:
	virtual void AddSelectDescriptors(fd_set& inSet,
		fd_set& outSet,
		fd_set& excSet,
		int& maxFD) = 0;
	virtual void SelectDescriptors(fd_set& inSet, fd_set& outSet, fd_set& excSet) = 0;
};

#endif
