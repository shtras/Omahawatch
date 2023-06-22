#include "Face.h"
#include "data.h"
#include "CurlWrapper.h"
#include <stdarg.h>
#include <time.h>

#ifdef _DEBUG
#define WATCH_ERR(f, ...) \
{ \
	watch_time_h time; \
	watch_time_get_current_time(&time); \
	int hour, min; \
	watch_time_get_hour(time, &hour); \
	watch_time_get_minute(time, &min); \
	setLastError("%.2d:%.2d " f, hour, min, __VA_ARGS__); \
}
#else
#define WATCH_ERR(...)
#endif

static const char* Months[] = {
		"Nul",
		"Jan",
		"Feb",
		"Mar",
		"Apr",
		"May",
		"Jun",
		"Jul",
		"Aug",
		"Sep",
		"Oct",
		"Nov",
		"Dec"
};

static const char* Weekdays[] = {
		"Nul",
		"Sun",
		"Mon",
		"Tue",
		"Wed",
		"Thu",
		"Fri",
		"Sat"
};

Face::Face(int width, int height):
	window_(NULL),
	bg_(NULL),
	layout_(NULL),
	handSec_(NULL),
	handMin_(NULL),
	handHour_(NULL),
	handsSecShadow_(NULL),
	handMinShadow_(NULL),
	handHourShadow_(NULL),
	weatherIcon_(NULL),
	batteryIcon_(NULL),
	sunsetIcon_(NULL),
	sunriseIcon_(NULL),
	animator_(NULL),
	weatherTimer_(NULL),
	locationTimeoutTimer_(NULL),
	locationManager_(NULL),
	weather_(new WeatherInfo()),
	width_(width),
	height_(height),
	listener_(NULL),
	ambient_(false),
	steps_(0),
	lastSteps_(0),
	lastTickDay_(-1),
	lastTickMinute_(-1),
	longitude_(0),
	latitude_(0),
	locationState_(-1),
	locationStateRequested_(-1)
{
}

Face::~Face()
{
	if (weatherTimer_) {
		Timer::GetInstance().DeleteTimer(weatherTimer_);
	}
	if (locationTimeoutTimer_) {
		Timer::GetInstance().DeleteTimer(locationTimeoutTimer_);
	}
	if (layout_) {
		evas_object_del(layout_);
	}
	if (bg_) {
		evas_object_del(bg_);
	}
	if (handSec_) {
		evas_object_del(handSec_);
	}
	if (handsSecShadow_) {
		evas_object_del(handsSecShadow_);
	}
	if (handMin_) {
		evas_object_del(handMin_);
	}
	if (handMinShadow_) {
		evas_object_del(handMinShadow_);
	}
	if (handHour_) {
		evas_object_del(handHour_);
	}
	if (handHourShadow_) {
		evas_object_del(handHourShadow_);
	}
	if (weatherIcon_) {
		evas_object_del(weatherIcon_);
	}
	if (batteryIcon_) {
		evas_object_del(batteryIcon_);
	}
	if (animator_) {
		ecore_animator_del(animator_);
	}
	if (listener_) {
		sensor_listener_unset_event_cb(listener_);
		sensor_listener_stop(listener_);
		sensor_destroy_listener(listener_);
	}
	if (locationManager_) {
		location_manager_unset_service_state_changed_cb(locationManager_);
		location_manager_stop(locationManager_);
		location_manager_destroy(locationManager_);
	}
	delete weather_;
}

void Face::weatherClickCallback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Face* face = (Face*)data;
	face->onWeatherClick();
}

void Face::onWeatherClick()
{
	if (!weather_) {
		return;
	}
	weather_->ToggleScale();
	updateWeatherText();
}

bool Face::Init()
{
	if (!createWindow()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create window");
		return false;
	}

	if (!createBg()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create background");
		return false;
	}

	if (!createSublayoutParts()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create sublayout parts");
		return false;
	}

	if (!createLayout()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create layout");
		return false;
	}

	if (!createParts()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create parts");
		return false;
	}

	animator_ = ecore_animator_add(Face::animatorCallback, this);

	if (!setupListeners()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to setup listeners");
		return false;
	}

	if (!setupLocation()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to setup location");
		return false;
	}

	return true;
}

int Face::updateLocation()
{
	double altitude;
	time_t timestamp;
	return location_manager_get_position(locationManager_, &altitude, &latitude_, &longitude_, &timestamp);
}

void Face::updateWeatherText()
{
	if (!weather_ || !weather_->Ready()) {
		return;
	}
	char text[64] = { 0, };
	weather_->GetString(text, sizeof(text));
	elm_object_part_text_set(layout_, "txt.weather", text);
}

#define Q(x)  #x
#define QUOTE(x)  Q(x)

void Face::updateWeather()
{
	WATCH_ERR("%s", "updw");
	char url[256];
	sprintf(url, "http://api.openweathermap.org/data/2.5/weather?lat=%.3f&lon=%.3f&APPID=" QUOTE(WEATHER_TOKEN), latitude_, longitude_);
	int err = 0;
	auto json = CurlWrapper::Get(url, &err);
	if (json.empty()) {
		WATCH_ERR("curl: %d", err);
		return;
	}

	bool res = weather_->FromJson(json.c_str());
	if (res) {
		updateWeatherText();

		char imagePath[PATH_MAX] = { 0, };
		data_get_resource_path(weather_->Icon(), imagePath, sizeof(imagePath));
		elm_image_file_set(weatherIcon_, imagePath, NULL);

		moveSunIcon(sunriseIcon_, weather_->Sunrise());
		evas_object_show(sunriseIcon_);

		moveSunIcon(sunsetIcon_, weather_->Sunset());
		evas_object_show(sunsetIcon_);
	} else {
		WATCH_ERR("%s", "jsnerr");
	}
	dlog_print(DLOG_DEBUG, LOG_TAG, "Weather: %s", json.c_str());
}

void Face::moveSunIcon(Evas_Object* icon, time_t time)
{
	struct tm* timeInfo = localtime(&time);
	int degree = (timeInfo->tm_hour > 12 ? timeInfo->tm_hour - 12 : timeInfo->tm_hour) * HOUR_ANGLE;
	degree += timeInfo->tm_min * HOUR_ANGLE / 60.0 - 90;
	double rsin = sin(degree * M_PI / 180.0);
	double rcos = cos(degree * M_PI / 180.0);
	int x = (BASE_WIDTH / 2 - SUN_ICON_WIDTH / 2) * (1 + rcos);
	int y = (BASE_HEIGHT / 2 - SUN_ICON_HEIGHT / 2) * (1 + rsin);
	evas_object_move(icon, x, y);
}

void Face::locationStateCallback(location_service_state_e state, void *data)
{
	Face* face = (Face*)data;
	face->onLocationState(state);
}

void Face::onLocationState(location_service_state_e state)
{
	if (locationTimeoutTimer_) {
		Timer::GetInstance().DeleteTimer(locationTimeoutTimer_);
		locationTimeoutTimer_ = NULL;
	}
	locationState_ = state;
	dlog_print(DLOG_DEBUG, LOG_TAG, "Location state change: %d", state);
	WATCH_ERR("CB %d", state);
	if (state == LOCATIONS_SERVICE_ENABLED) {
		int ret = updateLocation();
		if (ret != APP_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "updateLocation failed: %s", get_error_message(ret));
			WATCH_ERR("loc: %s", get_error_message(ret));
			return;
		}
		updateWeather();
		requestLocationServiceState(LOCATIONS_SERVICE_DISABLED);
	}
}

bool Face::weatherTimerFunc(void* data)
{
	Face* face = (Face*)data;
	face->onWeatherTimer();
	return true;
}

void Face::onWeatherTimer()
{
	WATCH_ERR("tmr %d %d", locationState_, locationStateRequested_);

	dlog_print(DLOG_DEBUG, LOG_TAG, "onWeatherTimer. State: %d Requested: %d", locationState_, locationStateRequested_);
	if (locationState_ == LOCATIONS_SERVICE_DISABLED) {
		requestLocationServiceState(LOCATIONS_SERVICE_ENABLED);
	} else {
		requestLocationServiceState(LOCATIONS_SERVICE_DISABLED);
	}
	WATCH_ERR("%s", "tmrend");
}

void Face::PauseAnimator()
{
	ecore_animator_freeze(animator_);
}

void Face::ResumeAnimator()
{
	ecore_animator_thaw(animator_);
}

Eina_Bool Face::animatorCallback(void *data)
{
	Face* face = (Face*)data;
	if (!face->onAnimator()) {
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

bool Face::onAnimator()
{
	watch_time_h time;
	int ret = watch_time_get_current_time(&time);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get current time. err = %d", ret);
		return true;
	}
	moveHands(time);

	watch_time_delete(time);
	return true;
}

void Face::listenerCallback(sensor_h sensorHanlder, sensor_event_s* event, void* data)
{
	Face* face = (Face*)data;
	face->onListener(sensorHanlder, event);
}

void Face::onListener(sensor_h sensorHanlder, sensor_event_s* event)
{
	// Assume pedometer. Extend later.
	steps_ = (int)event->values[0];
}

void Face::Tick(watch_time_h time)
{
	Timer::GetInstance().Tick();
	bool resetSensorCounters = false;
	int currDay = 0;
	int minute = 0;
	watch_time_get_day(time, &currDay);
	watch_time_get_minute(time, &minute);

	if (lastTickDay_ == -1 || currDay != lastTickDay_ || steps_ < lastSteps_) {
		resetSensorCounters = true;
	}
	lastTickDay_ = currDay;
	if (resetSensorCounters) {
		sensor_event_s event;
		int res = sensor_listener_read_data(listener_, &event);
		if (res == SENSOR_ERROR_NONE) {
			lastSteps_ = event.values[0];
			steps_ = lastSteps_;
			dlog_print(DLOG_INFO, LOG_TAG, "Counters reset");
		} else {
			lastTickDay_ = -1;
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed resetting last counters");
		}
	}

	moveHands(time);
	if (lastTickMinute_ != minute) {
		lastTickMinute_ = minute;
		updateTextFields();
		updateDate(time);
	}
}

bool Face::ToggleAmbient(bool ambient)
{
	ambient_ = ambient;

	if (ambient) {
		evas_object_hide(handSec_);
		evas_object_hide(handsSecShadow_);

		evas_object_color_set(handHour_, 150, 150, 150, 255);
		evas_object_color_set(handMin_, 150, 150, 150, 255);
		evas_object_color_set(weatherIcon_, 150, 150, 150, 255);
		evas_object_color_set(batteryIcon_, 150, 150, 150, 255);
		evas_object_color_set(sunsetIcon_, 150, 150, 150, 255);
		evas_object_color_set(sunriseIcon_, 150, 150, 150, 255);

		edje_color_class_set("dimmable", 100, 100, 100, 255, 100, 100, 100, 255, 100, 100, 100, 255);
	} else {
		evas_object_show(handSec_);
		evas_object_show(handsSecShadow_);

		evas_object_color_set(handHour_, 255, 255, 255, 255);
		evas_object_color_set(handMin_, 255, 255, 255, 255);
		evas_object_color_set(weatherIcon_, 255, 255, 255, 255);
		evas_object_color_set(batteryIcon_, 255, 255, 255, 255);
		evas_object_color_set(sunsetIcon_, 255, 255, 255, 255);
		evas_object_color_set(sunriseIcon_, 255, 255, 255, 255);

		edje_color_class_set("dimmable", 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255);
	}

	if (!setBg()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed changing background on ambient change");
		return false;
	}

	return true;
}

bool Face::createWindow()
{
	int ret = watch_app_get_elm_win(&window_);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get watch window. err = %d", ret);
		return false;
	}
	evas_object_resize(window_, width_, height_);
	evas_object_show(window_);

	return true;
}

bool Face::createBg()
{
	bg_ = elm_bg_add(window_);
	if (!bg_) {
		return false;
	}
	elm_bg_option_set(bg_, ELM_BG_OPTION_CENTER);

	evas_object_move(bg_, 0, 0);
	evas_object_resize(bg_, width_, height_);
	evas_object_show(bg_);

	if (!setBg()) {
		return false;
	}

	layout_ = elm_layout_add(bg_);

	return true;
}

bool Face::setBg()
{
	char path[PATH_MAX];

	if (ambient_) {
		data_get_resource_path(IMAGE_BG_BLACK, path, sizeof(path));
	} else {
		data_get_resource_path(IMAGE_BG, path, sizeof(path));
	}

	int ret = elm_bg_file_set(bg_, path, NULL);
	if (ret != EINA_TRUE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to set the background image");
		evas_object_del(bg_);
		return false;
	}
	return true;
}

bool Face::createLayout()
{
	char edjPath[PATH_MAX] = { 0, };

	data_get_resource_path(EDJ_FILE, edjPath, sizeof(edjPath));
	elm_layout_file_set(layout_, edjPath, "omaha");
	evas_object_size_hint_weight_set(layout_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(layout_);

	if (layout_ == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create layout from edc");
		return false;
	}

	evas_object_resize(layout_, BASE_WIDTH, BASE_HEIGHT);
	evas_object_show(layout_);

	return true;
}

Evas_Object* Face::createPart(const char* path, int x, int y, int width, int height)
{
	Evas_Object* part = elm_image_add(bg_);
	if (!part) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to add hand image");
		return NULL;
	}

	char imagePath[PATH_MAX] = { 0, };
	data_get_resource_path(path, imagePath, sizeof(imagePath));
	Eina_Bool ret = elm_image_file_set(part, imagePath, NULL);
	if (ret != EINA_TRUE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to set hand image %s", path);
		evas_object_del(part);
		return NULL;
	}

	evas_object_move(part, x, y);
	evas_object_resize(part, width, height);
	evas_object_show(part);

	return part;
}

bool Face::createSublayoutParts()
{
	if (!(weatherIcon_ = createPart("images/01d.png", 220, 50, 50, 50))) {
		return false;
	}
	if (!(batteryIcon_ = createPart("images/b100.png", 270, 180, 40, 20))) {
		return false;
	}
	return true;
}

bool Face::createParts()
{
	if (!(handHourShadow_ = createPart(IMAGE_HANDS_HOUR_SHADOW, (BASE_WIDTH / 2) - (HANDS_HOUR_WIDTH / 2), HANDS_HOUR_SHADOW_PADDING, HANDS_HOUR_WIDTH, HANDS_HOUR_HEIGHT))) {
		return false;
	}
	if (!(handHour_= createPart(IMAGE_HANDS_HOUR, (BASE_WIDTH / 2) - (HANDS_HOUR_WIDTH / 2), 0, HANDS_HOUR_WIDTH, HANDS_HOUR_HEIGHT))) {
		return false;
	}
	if (!(handMinShadow_ = createPart(IMAGE_HANDS_MIN_SHADOW, (BASE_WIDTH / 2) - (HANDS_MIN_WIDTH / 2), HANDS_MIN_SHADOW_PADDING, HANDS_MIN_WIDTH, HANDS_MIN_HEIGHT))) {
		return false;
	}
	if (!(handMin_ = createPart(IMAGE_HANDS_MIN, (BASE_WIDTH / 2) - (HANDS_MIN_WIDTH / 2), 0, HANDS_MIN_WIDTH, HANDS_MIN_HEIGHT))) {
		return false;
	}
	if (!(handsSecShadow_ = createPart(IMAGE_HANDS_SEC_SHADOW, (BASE_WIDTH / 2) - (HANDS_SEC_WIDTH / 2), HANDS_SEC_SHADOW_PADDING, HANDS_SEC_WIDTH, HANDS_SEC_HEIGHT))) {
		return false;
	}
	if (!(handSec_ = createPart(IMAGE_HANDS_SEC, (BASE_WIDTH / 2) - (HANDS_SEC_WIDTH / 2), 0, HANDS_SEC_WIDTH, HANDS_SEC_HEIGHT))) {
		return false;
	}
	if (!(sunsetIcon_ = createPart("images/sunset.png", 0, 0, SUN_ICON_WIDTH, SUN_ICON_HEIGHT))) {
		return false;
	}
	if (!(sunriseIcon_ = createPart("images/sunrise.png", 0, 0, SUN_ICON_WIDTH, SUN_ICON_HEIGHT))) {
		return false;
	}
	evas_object_hide(sunsetIcon_);
	evas_object_hide(sunriseIcon_);

	Evas_Object* tb = elm_entry_add(bg_);
	evas_object_move(tb, 202, 68);
	evas_object_resize(tb, 100, 80);
	evas_object_show(tb);
	evas_object_event_callback_add(tb, EVAS_CALLBACK_MOUSE_UP, Face::weatherClickCallback, this);

	return true;
}

bool Face::setupListeners()
{
	sensor_h sensorHanlder;
	sensor_get_default_sensor(SENSOR_HUMAN_PEDOMETER, &sensorHanlder);

	sensor_create_listener(sensorHanlder, &listener_);
	sensor_listener_set_option(listener_, SENSOR_OPTION_ALWAYS_ON);
	sensor_listener_set_event_cb(listener_, 1000, Face::listenerCallback, this);
	sensor_listener_start(listener_);

	return true;
}

bool Face::setupLocation()
{
	int ret = 0;
	ret = location_manager_create(LOCATIONS_METHOD_WPS, &locationManager_);
	if (ret == LOCATIONS_ERROR_NOT_SUPPORTED) {
		WATCH_ERR("%s", "not sup");
		ret = location_manager_create(LOCATIONS_METHOD_HYBRID, &locationManager_);
	}
	if (ret != LOCATIONS_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_create failed: %s", get_error_message(ret));
		return false;
	}
	ret = location_manager_set_service_state_changed_cb(locationManager_, Face::locationStateCallback, this);
	if (ret != LOCATIONS_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_set_service_state_changed_cb failed: %s", get_error_message(ret));
		return false;
	}

	if (!requestLocationServiceState(LOCATIONS_SERVICE_ENABLED)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "requestLocationServiceState failed");
		return false;
	}

	if (!weatherTimer_) {
		weatherTimer_ = Timer::GetInstance().AddTimer(10 * 60, weatherTimerFunc, this);
	}

	return true;
}

void Face::rotateHand(Evas_Object *hand, double degree, Evas_Coord cx, Evas_Coord cy)
{
	Evas_Map *m = NULL;

	m = evas_map_new(4);
	evas_map_util_points_populate_from_object(m, hand);
	evas_map_util_rotate(m, degree, cx, cy);
	evas_object_map_set(hand, m);
	evas_object_map_enable_set(hand, EINA_TRUE);
	evas_map_free(m);
}

void Face::moveHands(watch_time_h time)
{
	int msec = 0;
	int sec = 0;
	int min = 0;
	int hour = 0;
	double degree = 0.0f;

	watch_time_get_hour(time, &hour);
	watch_time_get_minute(time, &min);
	watch_time_get_second(time, &sec);
	watch_time_get_millisecond(time, &msec);

	degree = hour * HOUR_ANGLE;
	degree += min * HOUR_ANGLE / 60.0;
	rotateHand(handHour_, degree, (BASE_WIDTH / 2), (BASE_HEIGHT / 2));
	rotateHand(handHourShadow_, degree, (BASE_WIDTH / 2), (BASE_HEIGHT / 2) + HANDS_HOUR_SHADOW_PADDING);

	degree = min * MIN_ANGLE;
	degree += sec * MIN_ANGLE / 60.0;
	rotateHand(handMin_, degree, (BASE_WIDTH / 2), (BASE_HEIGHT / 2));
	rotateHand(handMinShadow_, degree, (BASE_WIDTH / 2), (BASE_HEIGHT / 2) + HANDS_MIN_SHADOW_PADDING);

	degree = sec * SEC_ANGLE;
	degree += msec * SEC_ANGLE / 1000.0;
	rotateHand(handSec_, degree, (BASE_WIDTH / 2), (BASE_HEIGHT / 2));
	rotateHand(handsSecShadow_, degree,  (BASE_WIDTH / 2), (BASE_HEIGHT / 2) + HANDS_SEC_SHADOW_PADDING);
}

void Face::updateDate(watch_time_h time)
{
	char fullDateStr[256];
	int hour, minute, month, day, weekDay;
	watch_time_get_month(time, &month);
	watch_time_get_day(time, &day);
	watch_time_get_day_of_week(time, &weekDay);
	sprintf(fullDateStr, "%s %s %d", Weekdays[weekDay], Months[month], day);
	if (!weather_ || !weather_->Ready()) {
		elm_object_part_text_set(layout_, "txt.date", fullDateStr);
		return;
	}
	time_t sunset = weather_->Sunset();
	struct tm sunsetInfo;
	sunsetInfo = *localtime(&sunset);
	time_t sunrise = weather_->Sunrise();
	struct tm sunriseInfo;
	sunriseInfo = *localtime(&sunrise);
	bool beforeSunrise = false;
	watch_time_get_hour24(time, &hour);
	watch_time_get_minute(time, &minute);
	if (hour > sunsetInfo.tm_hour || hour < sunriseInfo.tm_hour) {
		beforeSunrise = true;
	} else if (hour == sunsetInfo.tm_hour && minute > sunsetInfo.tm_min) {
		beforeSunrise = true;
	} else if (hour == sunriseInfo.tm_hour && minute <= sunriseInfo.tm_min) {
		beforeSunrise = true;
	}
	struct tm* nextEvent = beforeSunrise ? &sunriseInfo : &sunsetInfo;

	int dHour = nextEvent->tm_hour - hour;
	int dMinute = nextEvent->tm_min - minute;
	if (dMinute < 0) {
		dMinute += 60;
		--dHour;
	}
	if (dHour < 0) {
		dHour += 24;
	}

	const char* nextEventStr = beforeSunrise ? "Sunrise" : "Sunset";
	char fullNextEventStr[128] = {0};
	if (dHour == 0 && dMinute == 0) {
		sprintf(fullNextEventStr, "<br/>%s is now", nextEventStr);
	} else {
		sprintf(fullNextEventStr, "<br/>%s in<br/>%.2d:%.2d", nextEventStr, dHour, dMinute);
	}
	strcat(fullDateStr, fullNextEventStr);
	elm_object_part_text_set(layout_, "txt.date", fullDateStr);
}

void Face::updateTextFields()
{
	int batteryPercent = 0;

	int res = device_battery_get_percent(&batteryPercent);
	if (res) {
		batteryPercent = 0;
	}

	char imagePath[PATH_MAX] = { 0, };
	if (batteryPercent > 87) {
		data_get_resource_path("images/b100.png", imagePath, sizeof(imagePath));
	} else if (batteryPercent > 62) {
		data_get_resource_path("images/b75.png", imagePath, sizeof(imagePath));
	} else if (batteryPercent > 37) {
		data_get_resource_path("images/b50.png", imagePath, sizeof(imagePath));
	} else if (batteryPercent > 12) {
		data_get_resource_path("images/b25.png", imagePath, sizeof(imagePath));
	} else {
		data_get_resource_path("images/b0.png", imagePath, sizeof(imagePath));
	}
	elm_image_file_set(batteryIcon_, imagePath, NULL);


	char text[32] = { 0, };
	snprintf(text, sizeof(text), "%d%%", batteryPercent);
	elm_object_part_text_set(layout_, "txt.battery.num", text);
	updateTextField("txt.steps.num", steps_ - lastSteps_);
}

void Face::updateTextField(const char* fieldId, int value)
{
	char text[32] = { 0, };
	snprintf(text, sizeof(text), "%d", value);
	elm_object_part_text_set(layout_, fieldId, text);
}

void Face::setLastError(const char* fmt, ...)
{
	char msg[64];
	va_list args;
	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);
	string s = msg;
	errors_.push_back(s);
	if (errors_.size() > 12) {
		errors_.pop_front();
	}
	char allErrors[1024] = {0};
	for (const auto& itr: errors_) {
		strcat(allErrors, itr.c_str());
		strcat(allErrors, "<br/>");
	}
	elm_object_part_text_set(layout_, "txt.error", allErrors);
}

bool Face::LocationTimeoutCallback(void* data)
{
	Face* face = (Face*)data;
	face->onLocationTimeout();
	return false;
}

void Face::onLocationTimeout()
{
	WATCH_ERR("%s", "l t/o");
	locationStateRequested_ = locationState_ = 0;
	requestLocationServiceState(LOCATIONS_SERVICE_DISABLED);
	locationTimeoutTimer_ = NULL;
}

bool Face::requestLocationServiceState(location_service_state_e state)
{
	int ret;
	WATCH_ERR("Req: %d", state);
	if (locationStateRequested_ != locationState_) {
		dlog_print(DLOG_WARN, LOG_TAG, "%s Another request in progress", __FUNCTION__);
		WATCH_ERR("%s", "err1");
		return false;
	}
	if (state == LOCATIONS_SERVICE_ENABLED) {
		ret = location_manager_start(locationManager_);
		if (ret != LOCATIONS_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_start failed: %s", get_error_message(ret));
			WATCH_ERR("%s", "err2");
			return false;
		}
		if (!locationTimeoutTimer_) {
			locationTimeoutTimer_ = Timer::GetInstance().AddTimer(2 * 60, Face::LocationTimeoutCallback, this);
			if (!locationTimeoutTimer_) {
				WATCH_ERR("%s", "tmr fail");
			}
		}
	} else if (state == LOCATIONS_SERVICE_DISABLED) {
		ret = location_manager_stop(locationManager_);
		if (ret != LOCATIONS_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_stop failed: %s", get_error_message(ret));
			WATCH_ERR("%s", "err3");
			return false;
		}
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Unknown state: %d", state);
		WATCH_ERR("%s", "err4");
		return false;
	}
	locationStateRequested_ = state;
	return true;
}
