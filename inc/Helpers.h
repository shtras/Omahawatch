#ifndef _HELPERS_H_
#define _HELPERS_H_

template<class T>
class AutoDel
{
public:
	AutoDel(const T& t):
		t_(t)
	{
		dlog_print(DLOG_DEBUG, LOG_TAG, "created: %p", t_);
	}

	~AutoDel()
	{
		dlog_print(DLOG_DEBUG, LOG_TAG, "deleted: %p", t_);
		delete t_;
	}

	operator T() {return t_;}
	operator bool() {return t_ != NULL;}
private:
	T t_;
};

#endif
