/*
* Copyright 2020 Comcast Cable Communications Management, LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __VA_DEFINES_H__
#define __VA_DEFINES_H__

#include <stdint.h>
/* Properties to be read from config file */
#define SCENE                   "scene"
#define HUMAN_DETECT            "human_detect"
#define MOTION_MODE             "md_mode"
#define MOTION_SENSITIVITY1     "md_sensitivity1"

#define SCALE_FACTOR_YUV        1       /* yuv frame type should be scaled with 1 */
#define SCALE_FACTOR_ME1        4       /* me1 frame type should be scaled with 4 */

/* Enable run time debug logging */
extern int enable_debug;
#define RDK_LOG_DEBUG1 (enable_debug ? (RDK_LOG_INFO) : (RDK_LOG_DEBUG))
#define ENABLE_IVA_RDK_DEBUG_LOG_FILE   "/tmp/.enable_xvision_rdk_debug"

#define OD_RES(res)    			        (((res) & 0xFF00) >> 8)
#define OD_NUM(num)    			        ((num) & 0x00FF)
#define OD_MAX_NUM			        32  //we support maximum 16 objects detected in one frame

#define MAX_EVENTS_NUM 			        3
#define SLEEP_TIME_MAX			        100
#define CONFIG_PARAM_MAX_LENGTH                 256     /* Array size to store user setting params */
#define SLEEPTIMER                              100000  /*sleep time after reading one frame*/

#define NIGHT_MODE                              1       /* Flag used to identify Night mode status */
#define DAY_MODE                                0       /* Flag used to identify Day mode status */
#define DEFAULT_DN_MODE                         DAY_MODE/* Default day-night mode is day mode */
#ifdef OSI
#define _SUPPORT_OBJECT_DETECTION_IV_
#endif
/* Event types */
typedef enum
 {
   EVENT_TYPE_INPUT1,
   EVENT_TYPE_INPUT2,
   EVENT_TYPE_MOTION,
   EVENT_TYPE_PIR,
   EVENT_TYPE_AUDIO,
   EVENT_TYPE_HTTP,
   EVENT_TYPE_PERIOD,
   EVENT_TYPE_CONTINUE,
   EVENT_TYPE_INPUT3,
   EVENT_TYPE_INPUT4,
   EVENT_TYPE_TAMPER,              //et=10
   EVENT_TYPE_PEOPLE,              //et=11
   EVENT_TYPE_MAX,
   EVENT_TYPE_UNKNOWN
 } EventType;

/* Aspect Ratio */
enum
 {
   ASPECT_RATIO_MIXED = 0,
   ASPECT_RATIO_16_9 = 1,
   ASPECT_RATIO_4_3 = 2,
   ASPECT_RATIO_16_6 = 3
 };

/* Resolution */
enum
 {
   IMG_RES_MIN,
   IMG_RES_QQVGA,  //* 160 x 120 */ 1 - mixed
   IMG_RES_QVGA,           //* 320 x 240 */ 2
   IMG_RES_VGA,            //* 640 x 480 */ 3
   IMG_RES_SXGA,           //* 1280 x 720 */ 4
   IMG_RES_1080P,  //* 1920 x 1080 */ 5
   IMG_RES_CIF,      //* 352 x 288 */ 6
   IMG_RES_4CIF,     //* 704 x 576 */ 7
   IMG_RES_480P,     //* 720 x 480 */ 8
   IMG_RES_576P,     //* 720 x 576 */ 9
   IMG_RES_720P,     //* 1280 x 720 */ 10
   IMG_RES_QQQHD=11,   //* 160 x 90 */ 11 - 16:9
   IMG_RES_QQHD,       //* 320 x 180 */ 12
   IMG_RES_QHD,        //* 640 x 360 */ 13
   IMG_RES_HD,         //* 1280 x 720 */ 14
   IMG_RES_UHD,        //* 1920 x 1080 */ 15
   IMG_RES_960P,       //* 960 x 720 */ 16
   IMG_RES_XGA,      //* 1024 x 720 */ 17
   IMG_RES_1440_1080,      //* 1440 x 1080 */
   IMG_RES_UHD_QQVGA=21,   // 160x60 - 16:6
   IMG_RES_UHD_QVGA,               // 320x120
   IMG_RES_UHD_VGA,                // 640x240
   IMG_RES_UHD_HD,         // 1280x480
   IMG_RES_UHD_1080P,              // 1920x720
   IMG_RES_1440P,  //* 2560 x 1440 */
   IMG_RES_NUM
 };

enum XcvStatus
 {
   XCV_SUCCESS = 0,
   XCV_FAILURE = -1,
   XCV_OTHER = 3,
 };

enum {
        OD_TYPE_UNKNOW,
        OD_TYPE_HUMAN,
        OD_TYPE_FACE,
        OD_TYPE_CAR,
        OD_TYPE_MAX
};

enum
{
        OD_RAW_RES_QVGA,     //320x240
        OD_RAW_RES_QHD,   //640x360, or 640x480
        OD_RAW_RES_HD,     //1280x720, or 940x720
        OD_RAW_RES_MAX
};

typedef struct vai_object
{
        uint16_t   od_id; // For tracking
        uint8_t   type;     //object type
        uint8_t   confidence;   //the confidence on the current detection result
        uint16_t   start_x; // The top-left coordinate of the object
        uint16_t   start_y; // The top-left coordinate of the object    
        uint16_t   width; // The width of the object
        uint16_t   height;// The height of the object
} vai_object_t;

typedef struct vai_result
{
        uint64_t   timestamp;
        uint16_t   num;         //number of objects detected
        vai_object_t vai_objects[OD_MAX_NUM];   // The object result is placed by od_id and save from 0 -> max
#ifdef _SUPPORT_OBJECT_DETECTION_IV_
        uint32_t   event_type;                  // Event type
        float   motion_level;                   // A percent of pixels under motion, range 0.0f~100.0f
        float   motion_level_raw;               // A percent of pixels under motion, range 0.0f~100.0f
#endif
} vai_result_t;

#endif
