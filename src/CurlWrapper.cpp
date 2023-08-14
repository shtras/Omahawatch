#include "CurlWrapper.h"

#include <functional>
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

class finally
{
public:
	explicit finally(std::function<void(void)>&& f) noexcept:f_(std::move(f)){}
	~finally() noexcept {f_();}
private:
	std::function<void(void)> f_;
};

std::string CurlWrapper::Get(const std::string& url, int& err, bool useProxy/* = true*/)
{
	CURL *curl;
	CURLcode curlErr;
	curl = curl_easy_init();
	char *proxyAddress = nullptr;
	connection_h connection;
	int connErr;
	connErr = connection_create(&connection);
	finally f([&curl, &connection, &proxyAddress]() {
		curl_easy_cleanup(curl);
		connection_destroy(connection);
		free(proxyAddress);
		dlog_print(DLOG_DEBUG, LOG_TAG, "Curl cleanup");
	});
	if (connErr != CONNECTION_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR1 %s", get_error_message(connErr));
		err = 0b1000000000000000 | connErr;
		return NULL;
	}
	std::string res;
	StringContext ctx(res);
	dlog_print(DLOG_DEBUG, LOG_TAG, "%s", url.c_str());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &StringContext::WriteCallback);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

	if (useProxy) {
		connErr = connection_get_proxy(connection, CONNECTION_ADDRESS_FAMILY_IPV4, &proxyAddress);
		if (connErr != CONNECTION_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "ERROR1.3 %s", get_error_message(connErr));
			err = 0b0100000000000000 | connErr;
			return "";
		}
		if (proxyAddress && *proxyAddress) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Using proxy: %s", proxyAddress);
			curl_easy_setopt(curl, CURLOPT_PROXY, proxyAddress);
		} else {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Got empty proxy. Ignoring");
		}
	}

	curlErr = curl_easy_perform(curl);
	if (curlErr == CURLE_OPERATION_TIMEDOUT && useProxy) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Curl timed out with proxy. Trying without...");
		std::string res = Get(url, err, false);
		err |= 0b0010000000000000;
		return res;
	} else if (curlErr != CURLE_OK) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR2 %d %d", curlErr, useProxy);
		err |= curlErr;
		res = "";
	}

	return res;
}

