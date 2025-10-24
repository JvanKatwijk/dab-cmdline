
#pragma once

#include	<stdio.h>
#include	<stdint.h>
#include	<vector>
#include	<thread>
#include	<atomic>
#include	"dab-constants.h"
#include	"tiiQueue.h"
#include	"cacheElement.h"
#include	"tii-reader.h"

class	tiiHandler {

public:
	tiiHandler	();

	~tiiHandler	();
void	start		(uint32_t);
void	stop		();
void	add		(tiiData theData);
cacheElement	deliver	(int);
int	nrTransmitters	();
private:

	tiiReader	theReader;
	std::vector<cacheElement> the_dataBase;
	tiiQueue	theBuffer;
	std::vector<cacheElement> tiiTable;
	bool		has_dataBase;
	bool	known		(tiiData &);
	void	run		();
	std::thread	threadHandle;
	std::atomic<bool>	running;
	cacheElement *	lookup	(tiiData &);
	uint32_t	currentEid;
};


