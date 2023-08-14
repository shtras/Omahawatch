#ifndef CURLWRAPPER_H_
#define CURLWRAPPER_H_
#include <stdio.h>
#include <string>

class CurlWrapper {
public:
	void Test();
	static std::string Get(const std::string& url, int& err, bool useProxy = true);
private:

};

#endif /* CURLWRAPPER_H_ */
