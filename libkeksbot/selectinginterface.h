#ifndef SELECTINGINTERFACE_H
#define SELECTINGINTERFACE_H

#include <sys/select.h>

class SelectingInterface
{
public:
	virtual void AddSelectDescriptors(fd_set& inSet, fd_set& outSet, int& maxFD) = 0;
	virtual void SelectDescriptors(fd_set& inSet, fd_set& outSet) = 0;
};

#endif
