#include "CurlWrapper.h"

#include <curl/curl.h>
#include <net_connection.h>
#include <dlog.h>
#include "omahawatch.h"

CurlWrapper::CurlWrapper()
{


}

CurlWrapper::~CurlWrapper()
{

}
struct MemoryStruct {
  char *memory;
  size_t size;
};
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = new char[realsize + 1];
  mem->size = realsize;

  memcpy(mem->memory, contents, realsize);
  mem->memory[mem->size] = 0;

  return realsize;
}

char* CurlWrapper::Get_d(const char* url, int* err/* = NULL*/)
{
	CURL *curl;
	CURLcode curl_err;
	curl = curl_easy_init();
	if (err) {
		*err = 0;
	}
	connection_h connection;
	int conn_err;
	conn_err = connection_create(&connection);
	if (conn_err != CONNECTION_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR1 %s", get_error_message(conn_err));
		*err = 0x80000000 | conn_err;
		return NULL;
	}
	struct MemoryStruct chunk;

	chunk.memory = NULL;
	chunk.size = 0;
	dlog_print(DLOG_DEBUG, LOG_TAG, "%s", url);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	char *proxy_address;
	conn_err = connection_get_proxy(connection, CONNECTION_ADDRESS_FAMILY_IPV4, &proxy_address);
	if (conn_err != CONNECTION_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR1.3 %s", get_error_message(conn_err));
		*err = 0x81000000 | conn_err;
	    return NULL;
	}
	curl_easy_setopt(curl, CURLOPT_PROXY, proxy_address);
	dlog_print(DLOG_DEBUG, LOG_TAG, "Proxy: %s", proxy_address);

	curl_err = curl_easy_perform(curl);
	if (curl_err != CURLE_OK) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR2 %s %d", get_error_message(curl_err), curl_err);
		curl_easy_cleanup(curl);
		connection_destroy(connection);
		if (err) {
			*err = curl_err;
		}
		return NULL;
	}
	//dlog_print(DLOG_INFO, LOG_TAG, chunk.memory);


	curl_easy_cleanup(curl);
	connection_destroy(connection);
	return chunk.memory;
}

