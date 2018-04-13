#ifndef CURLWRAPPER_H_
#define CURLWRAPPER_H_
#include <stdio.h>

class CurlWrapper {
public:
	CurlWrapper();
	~CurlWrapper();

	void Test();
	static char* Get_d(const char* url, int* err = NULL);
private:
};

#endif /* CURLWRAPPER_H_ */
