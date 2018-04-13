#include "Timer.h"

Timer& Timer::GetInstance()
{
	static Timer instance;
	return instance;
}

Timer::Timer():
	lastTimestamp_((int)ecore_time_unix_get())
{

}

Timer::~Timer()
{
	for (auto itr: events_) {
		delete itr;
	}
}


Timer::TimerHandle Timer::AddTimer(int seconds, TimerCallback cb, void* data)
{
	advanceTime();
	Event* event = new Event();
	event->cb = cb;
	event->time = lastTimestamp_ + seconds;
	event->interval = seconds;
	event->data = data;
	addEvent(event);
	return event;
}

void Timer::DeleteTimer(Timer::TimerHandle handle)
{
	advanceTime();
	for (auto itr = events_.begin(); itr != events_.end(); ++itr) {
		if (*itr == handle) {
			itr = events_.erase(itr);
			delete handle;
			break;
		}
	}
}

void Timer::Tick()
{
	advanceTime();
	while (events_.size() > 0 && events_.front()->time <= lastTimestamp_) {
		Event* event = events_.front();
		events_.pop_front();
		bool recur = event->cb(event->data);
		if (recur) {
			event->time = lastTimestamp_ + event->interval;
			addEvent(event);
		} else {
			delete event;
		}
	}
}

void Timer::advanceTime()
{
	lastTimestamp_ = (int)ecore_time_unix_get();
}

void Timer::addEvent(Event* event)
{
	bool found = false;
	for (auto itr = events_.begin(); itr != events_.end(); ++itr) {
		if ((*itr)->time > event->time) {
			events_.insert(itr, event);
			found = true;
			break;
		}
	}
	if (!found) {
		events_.push_back(event);
	}
}
