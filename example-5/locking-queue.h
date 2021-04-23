#
/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of terminal-DAB
 *
 *    terminal-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    terminal-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with terminal-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__LOCKING_QUEUE__
#define	__LOCKING_QUEUE__
#include	<queue>
#include        <thread>
#include        <mutex>
#include        <chrono>
#include        <condition_variable>

template <class job>
class lockingQueue {
private:
	std::queue <job> theQueue;
	mutex	mtx;
	condition_variable cv;
public:
		lockingQueue	()	{} 

		~lockingQueue	()	{}

	void	push		(job content) {
	 	theQueue. push (content);
		std::unique_lock<mutex> lck (mtx);
		cv. notify_one ();
	}

	bool	pop		(int delay, job *out) {
		unique_lock <mutex> lck (mtx);
	        job xxx;
		if (theQueue. size () == 0) {
	   	   auto now = std::chrono::system_clock:: now ();
	           cv. wait_until (lck, now + std::chrono::milliseconds (delay));
		}
	        if (theQueue. size () == 0)
	           return false;
	        xxx		= theQueue. front ();
	        *out		= xxx;
	        theQueue. pop ();
	        return true;
	}
};
#endif

