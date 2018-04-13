#ifndef _FACE_H_
#define _FACE_H_

#include <Elementary.h>
#include <efl_extension.h>
#include <watch_app.h>
#include <watch_app_efl.h>
#include <app.h>
#include <dlog.h>
#include <device/battery.h>
#include <omahawatch.h>
#include <sensor.h>
#include <locations.h>
#include "Timer.h"
#include <list>
#include <string>
using namespace std;

class Face {
public:
	Face(int width, int height);
	~Face();

	bool Init();
	void Tick(watch_time_h time);
	bool ToggleAmbient(bool ambient);
	void PauseAnimator();
	void ResumeAnimator();
private:
	bool createWindow();
	bool createBg();
	bool createLayout();
	bool createSublayoutParts();
	bool createParts();
	Evas_Object* createPart(const char* path, int x, int y, int width, int height);
	bool setupListeners();
	bool setupLocation();

	bool setBg();

	static Eina_Bool animatorCallback(void *data);
	bool onAnimator();

	static void listenerCallback(sensor_h sensorHanlder, sensor_event_s* event, void* data);
	void onListener(sensor_h sensorHanlder, sensor_event_s* event);

	static void locationStateCallback(location_service_state_e state, void *data);
	void onLocationState(location_service_state_e state);

	void moveHands(watch_time_h time);
	void rotateHand(Evas_Object *hand, double degree, Evas_Coord cx, Evas_Coord cy);

	void updateTextFields();
	void updateTextField(const char* fieldId, int value);

	static bool weatherTimerFunc(void* data);
	void onWeatherTimer();

	static bool LocationTimeoutCallback(void* data);
	void onLocationTimeout();

	int updateLocation();
	void updateWeather();

	bool requestLocationServiceState(location_service_state_e state);

	void setLastError(const char* fmt, ...);
	void moveSunIcon(Evas_Object* icon, time_t time);

	Evas_Object* window_;
	Evas_Object* bg_;
	Evas_Object* layout_;
	Evas_Object* handSec_;
	Evas_Object* handMin_;
	Evas_Object* handHour_;
	Evas_Object* handsSecShadow_;
	Evas_Object* handMinShadow_;
	Evas_Object* handHourShadow_;
	Evas_Object* weatherIcon_;
	Evas_Object* batteryIcon_;
	Evas_Object* sunsetIcon_;
	Evas_Object* sunriseIcon_;
	Ecore_Animator *animator_;
	Timer::TimerHandle weatherTimer_;
	Timer::TimerHandle locationTimeoutTimer_;

	location_manager_h locationManager_;

	int width_;
	int height_;

	sensor_listener_h listener_;
	bool ambient_;

	int steps_;
	int calories_;

	int lastSteps_;
	int lastCalories_;

	int lastTickDay_;
	double longitude_;
	double latitude_;

	int locationState_;
	int locationStateRequested_;

	list<string> errors_;
};

#endif /* FACE_H_ */
