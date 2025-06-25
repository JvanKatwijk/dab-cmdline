

#include	"tiiQueue.h"

	tiiQueue::tiiQueue	() {}

	tiiQueue::~tiiQueue	() {}

void	tiiQueue::push 	(tiiData item) {
	   std::unique_lock<std::mutex> lock(m_mutex);
	   m_queue. push (item);
}

bool	tiiQueue::empty	() {
	   std::unique_lock<std::mutex> lock(m_mutex);
	   return m_queue. size () == 0;
}

tiiData	tiiQueue::pop () {
	   std::unique_lock<std::mutex> lock(m_mutex);
           tiiData item = m_queue.front();
           m_queue.pop();
	   return item;
}

