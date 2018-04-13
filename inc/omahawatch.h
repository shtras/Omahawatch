#ifndef _OMAHAWATCH_H_
#define _OMAHAWATCH_H_

#if !defined(PACKAGE)
#define PACKAGE "net.shtras.omahawatch"
#endif

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "omahawatch"

#define IMAGE_BG "images/chrono_clock_bg.png"
#define IMAGE_BG_BLACK "images/chrono_clock_bg_black.png"

#define IMAGE_HANDS_SEC "images/chrono_hand_sec.png"
#define IMAGE_HANDS_MIN "images/chrono_hand_min.png"
#define IMAGE_HANDS_HOUR "images/chrono_hand_hour.png"
#define IMAGE_HANDS_SEC_SHADOW "images/chrono_hand_sec_shadow.png"
#define IMAGE_HANDS_MIN_SHADOW "images/chrono_hand_min_shadow.png"
#define IMAGE_HANDS_HOUR_SHADOW "images/chrono_hand_hour_shadow.png"

#define EDJ_FILE "edje/main.edj"


#define PARTS_TYPE_NUM 6

/* Angle */
#define HOUR_ANGLE 30
#define MIN_ANGLE 6
#define SEC_ANGLE 6

/* Layout */
#define BASE_WIDTH 360
#define BASE_HEIGHT 360

#define HANDS_SEC_WIDTH 28
#define HANDS_SEC_HEIGHT 360
#define HANDS_MIN_WIDTH 28
#define HANDS_MIN_HEIGHT 360
#define HANDS_HOUR_WIDTH 28
#define HANDS_HOUR_HEIGHT 360

#define HANDS_SEC_SHADOW_PADDING 3
#define HANDS_MIN_SHADOW_PADDING 6
#define HANDS_HOUR_SHADOW_PADDING 6

#endif
