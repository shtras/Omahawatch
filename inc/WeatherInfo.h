#ifndef _WEATHERINFO_H_
#define _WEATHERINFO_H_
#include <json-glib/json-glib.h>

class WeatherInfo
{
public:
	WeatherInfo();
	~WeatherInfo();

	bool FromJson(const char* json);
	const char* Icon() {return icon_;}
	time_t Sunset() {return sunset_;}
	time_t Sunrise() {return sunrise_;}
	void GetString(char* str, int len);
	bool Ready() {return ready_;}
	void ToggleScale() { celsius_ = !celsius_; }
private:
	JsonNode* getNode(JsonObject* parent, const char* name);
	JsonNode* getNodePath(JsonObject* parent, const char* path);
	JsonParser* jsonParser_;
	float temp_;
	char location_[128];
	char icon_[64];
	time_t sunset_;
	time_t sunrise_;
	bool ready_;
	bool celsius_;
	int updateHour_;
	int updateMinute_;
};

#endif
