#
#ifndef	__SEMAPHORE
#define	__SEMAPHORE

#include	<thread>
#include	<mutex>
#include	<condition_variable>

class	Semaphore {
private:
	mutex mtx;
	condition_variable cv;
	int count;
public:
	Semaphore (int count_ = 0) : count {count_} {}

void	notify (void) {
	std::unique_lock<mutex>lck (mtx);
	++count;
	cv. notify_one ();
}

void	wait (void) {
	unique_lock <mutex> lck (mtx);
	while (count == 0) {
	   cv. wait (lck);
	}
	-- count;
}
};
#endif
