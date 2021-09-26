#include "WeatherInfo.h"
#include <Elementary.h>
#include <watch_app.h>
#include <dlog.h>
#include <stdio.h>
#include <string.h>
#include "omahawatch.h"

WeatherInfo::WeatherInfo():
	jsonParser_(NULL),
	temp_(0),
	sunset_(0),
	sunrise_(0),
	ready_(false),
	celsius_(false),
	updateHour_(0),
	updateMinute_(0)
{
	location_[0] = '\0';
}

WeatherInfo::~WeatherInfo()
{
	if (jsonParser_) {
		g_object_unref(jsonParser_);
	}
}

JsonNode* WeatherInfo::getNode(JsonObject* parent, const char* name)
{
	guint size = json_object_get_size (parent);
	GList* keysList = json_object_get_members (parent);
	GList* valList = json_object_get_values (parent);
	JsonNode* res = NULL;
	for (int i=0; i<size; ++i) {
		if (!keysList || !valList) {
			break;
		}
	    if (!strcmp((gchar*)keysList->data, name)) {
	    	res = (JsonNode*)valList->data;
	    	break;
	    }
		keysList = g_list_next(keysList);
		valList = g_list_next(valList);
	}
	g_list_free(keysList);
	g_list_free(valList);
	return res;
}

JsonNode* WeatherInfo::getNodePath(JsonObject* parent, const char* path)
{
	char workPath[128];
	strcpy(workPath, path);
	char* pathItr = strtok(workPath, "/");
	JsonObject* objItr = parent;
	JsonNode* nodeItr = NULL;
	while (pathItr) {
		int idx = -1;
		char* arr = strchr(pathItr, '[');
		if (arr) {
			sscanf(arr, "[%d]", &idx);
			*arr = '\0';
		}
		if (!objItr) {
			return NULL;
		}
		nodeItr = getNode(objItr, pathItr);
		if (!nodeItr) {
			return NULL;
		}
		if (idx > -1) {
			JsonArray* jsonArray = json_node_get_array(nodeItr);
			if (!jsonArray) {
				return NULL;
			}
			guint arraySize = json_array_get_length(jsonArray);
			if (arraySize < idx + 1) {
				return NULL;
			}
			nodeItr = json_array_get_element(jsonArray,idx);
		}
		objItr = json_node_get_object(nodeItr);
		pathItr = strtok(NULL, "/");
	}
	return nodeItr;
}

bool WeatherInfo::FromJson(const char* json)
{
	GError *error = NULL;
	if (jsonParser_) {
		g_object_unref(jsonParser_);
	}
	jsonParser_ = json_parser_new();
	json_parser_load_from_data(jsonParser_, json, strlen(json), &error);
	if (error) {
		g_error_free(error);
		return false;
	}

	JsonNode *root;
    root = json_parser_get_root(jsonParser_);
    if (!root) {
    	return false;
    }
    if (JSON_NODE_TYPE(root) != JSON_NODE_OBJECT) {
    	return false;
    }
    JsonObject* rootObj = json_node_get_object(root);

    // Name
    JsonNode* nameNode = getNode(rootObj, "name");
    if (!nameNode || (json_node_get_value_type(nameNode) != G_TYPE_STRING)) {
    	return false;
    }
	gchar* name = json_node_dup_string(nameNode);
	strncpy(location_, name, sizeof(location_) / sizeof(location_[0]) - 1);
	g_free(name);
	if (strlen(location_) > 20) {
		location_[17] = '.';
		location_[18] = '.';
		location_[19] = '.';
		location_[20] = '\0';
	}

	// Icon
	JsonNode* iconNode = getNodePath(rootObj, "weather[0]/icon");
    if (!iconNode || (json_node_get_value_type(iconNode) != G_TYPE_STRING)) {
    	 return false;
    }
	gchar* icon = json_node_dup_string(iconNode);
	sprintf(icon_, "images/%s.png", icon);
	g_free(icon);

	// Temp
    JsonNode* tempNode = getNodePath(rootObj, "main/temp");
    if (!tempNode || (json_node_get_value_type(tempNode) != G_TYPE_DOUBLE)) {
    	return false;
    }
    temp_ = json_node_get_double(tempNode) - 273.0f;

    // Sunset
    JsonNode* sunsetNode = getNodePath(rootObj, "sys/sunset");
    if (!sunsetNode ||  (json_node_get_value_type(sunsetNode) != G_TYPE_INT64)) {
    	return false;
    }
    sunset_ = (time_t)json_node_get_int(sunsetNode);

    // Sunrise
    JsonNode* sunriseNode = getNodePath(rootObj, "sys/sunrise");
    if (!sunriseNode ||  (json_node_get_value_type(sunriseNode) != G_TYPE_INT64)) {
    	return false;
    }
    sunrise_ = (time_t)json_node_get_int(sunriseNode);

    g_object_unref(jsonParser_);
    jsonParser_ = NULL;

    watch_time_h time;
	int ret = watch_time_get_current_time(&time);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get current time. err = %d", ret);
		return false;
	}

	watch_time_get_hour(time, &updateHour_);
	watch_time_get_minute(time, &updateMinute_);
	watch_time_delete(time);

    ready_ = true;
    return true;
}

void WeatherInfo::GetString(char* str, int len)
{
	*str = '\0';
	int temp = (int)temp_;
	if (!celsius_) {
		temp = temp * 9.0f / 5.0f + 32.0f;
	}
	snprintf(str, len, "%s<br/>%.2d:%.2d: %d°%c", location_, updateHour_, updateMinute_, temp, (celsius_ ? 'C' : 'F'));
}
