#ifndef _TIMER_H_
#define _TIMER_H_
#include "omahawatch.h"
#include <Elementary.h>
#include <dlog.h>
#include <list>
using namespace std;

class Timer {
	struct Event;
public:
	typedef bool(*TimerCallback)(void *data);
	typedef Timer::Event* TimerHandle;

	static Timer& GetInstance();

	TimerHandle AddTimer(int seconds, TimerCallback cb, void* data);
	void DeleteTimer(TimerHandle handle);
	void Tick();
private:
	Timer();
	~Timer();

	struct Event
	{
		int time;
		int interval;
		TimerCallback cb;
		void* data;
	};

	void advanceTime();
	void addEvent(Event* event);

	list<Event*> events_;
	int lastTimestamp_;
};

#endif
