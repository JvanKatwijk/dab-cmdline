
#pragma once

//	C++ implementation of the threadsafe queue
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include	"dab-constants.h"

class	tiiQueue {
private:
	std::queue<tiiData> m_queue;
	std::mutex m_mutex;

public:

		tiiQueue	();
		~tiiQueue	();
	void	push		(tiiData item);
	bool	empty		();
	tiiData pop 		();
};

