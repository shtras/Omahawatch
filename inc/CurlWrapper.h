#ifndef CURLWRAPPER_H_
#define CURLWRAPPER_H_
#include <stdio.h>
#include <string>

class CurlWrapper {
public:
	void Test();
	static std::string Get(const char* url, int* err);
private:

};

#endif /* CURLWRAPPER_H_ */
