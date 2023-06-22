#include "CurlWrapper.h"

#include <curl/curl.h>
#include <net_connection.h>
#include <dlog.h>
#include "omahawatch.h"

class StringContext
{
public:
    explicit StringContext(std::string& str) noexcept: data_(str)
	{
	    str.reserve(2 << 14);
	}

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        auto ctx = static_cast<StringContext*>(userp);
        size *= nmemb;
        ctx->data_.append(static_cast<const char*>(contents), size);
        return size;
    }

private:
    std::string& data_;
};

class CurlRAII
{
public:
	CurlRAII(CURL* curl, connection_h connection):
		curl_(curl), connection_(connection)
	{
	}

	~CurlRAII()
	{
		curl_easy_cleanup(curl_);
		connection_destroy(connection_);
	}
private:
	CURL* curl_;
	connection_h connection_;
};

std::string CurlWrapper::Get(const char* url, int* err, bool useProxy/* = true*/)
{
	CURL *curl;
	CURLcode curl_err;
	curl = curl_easy_init();
	*err = 0;
	connection_h connection;
	int conn_err;
	conn_err = connection_create(&connection);
	CurlRAII raii{curl, connection};
	if (conn_err != CONNECTION_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR1 %s", get_error_message(conn_err));
		*err = 0x80000000 | conn_err;
		return NULL;
	}
	std::string res;
	StringContext ctx(res);
	dlog_print(DLOG_DEBUG, LOG_TAG, "%s", url);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &StringContext::WriteCallback);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

	if (useProxy) {
		char *proxy_address;
		conn_err = connection_get_proxy(connection, CONNECTION_ADDRESS_FAMILY_IPV4, &proxy_address);
		if (conn_err != CONNECTION_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "ERROR1.3 %s", get_error_message(conn_err));
			*err = 0x81000000 | conn_err;
			return "";
		}
		curl_easy_setopt(curl, CURLOPT_PROXY, proxy_address);
		dlog_print(DLOG_DEBUG, LOG_TAG, "Proxy: %s", proxy_address);
	}

	curl_err = curl_easy_perform(curl);
	if (curl_err == CURLE_OPERATION_TIMEDOUT && useProxy) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Curl timed out with proxy. Trying without...");
		return Get(url, err, false);
	} else if (curl_err != CURLE_OK) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR2 %d", curl_err);
		*err = curl_err;
		res = "";
	}

	return res;
}

