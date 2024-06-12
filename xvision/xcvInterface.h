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

#ifndef _XCVINTERFACE_H_
#define _XCVINTERFACE_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef OSI
#include "conf_sec.h"
#ifdef __cplusplus
extern "C" {
#endif
  #include "sc_tool.h"
  #include "config_api.h"
  #include "PRO_file.h"
  #ifdef __cplusplus
 }
#endif
#endif

#ifdef _HAS_XSTREAM_
#ifdef __cplusplus
extern "C" {
#endif
 #include "sysUtils.h"
 #ifdef __cplusplus
 }
#endif
#endif

#include "va_defines.h"
#include "iavInterface.h"

#include "rdk_debug.h"

#ifdef RTMSG
#include "rtConnection.h"
#include "rtLog.h"
#include "rtMessage.h"
#define APP1 "APP1"
#define APP1_ADDRESS "tcp://127.0.0.1:10001"
#define DYNAMIC_LOG_REQ_RES_TOPIC       "RDKC.ENABLE_DYNAMIC_LOG"
#endif
#include "SmartMetadata.h"
#ifdef ENABLE_TEST_HARNESS
#include "THInterface.h"
#endif

#define MAX_CONFIG_LENGTH 512
#include "telemetry_busmessage_sender.h"

class xcvInterface
 {
   private:
    static double od_scale_table[OD_RAW_RES_MAX][IMG_RES_NUM];
    static int od_crop_table[OD_RAW_RES_MAX][IMG_RES_NUM];
    static int od_crop_max_width[OD_RAW_RES_MAX];
    static vai_result_t* vai_result;
#ifdef ENABLE_TEST_HARNESS
    static THInterface *th_interface;
#endif

#ifdef RTMSG
    static rtError err;
    static rtMessage m;
    static rtConnection connectionRecv;
    static rtConnection connectionSend;
    static bool rtmessageXCVThreadExit;
    static volatile bool smartTnEnabled;
    static volatile bool hasROIChanged;
    static volatile bool hasDOIChanged;

    //dynamic log thread
    static pthread_t rtMessageRecvThread;
    static void smtTnOnMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
    static void dynLogOnMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
    static void onMsgDOIConfRefresh(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
#ifdef _ROI_ENABLED_
    static void onMsgROIConfRefresh(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
#endif
    static void* receive_rtmessage(void *args);

#endif

   public:
    static char doibitmapPath[MAX_CONFIG_LENGTH];
    static bool doienabled;
    static int doiBitmapThreshold;
#ifdef _ROI_ENABLED_
    static char roiCoords[MAX_CONFIG_LENGTH];
#endif

    /* Constructor */
    xcvInterface();
    /* Print object */
    static int print_object(iObject *object);
    /*Print Event */
    static int print_event(iEvent *event);
    /* Get date and time */
    static int get_iDateTime(iDateTime *dateTime);
    /* Dump raw data */
    static int dump_raw_data(unsigned char *y_addr, unsigned char *uv_addr, int width, int height);
    /* Initialize rtMessage */
    static int rtMessageInit();
    /* Notify CVR via rtMessage */
    static int notifyCVR(uint64_t timestamp, uint32_t event_type, float motion_level_raw, char* curr_time);
    /* Notify CVR via rtMessage */
    static int notifyCVR(char *vaEngineVersion, uint64_t timestamp, uint32_t event_type, float motion_level_raw, float motionScore, uint32_t boundingBoxXOrd, uint32_t boundingBoxYOrd, uint32_t boundingBoxHeight, uint32_t boundingBoxWidth, char* curr_time);
    /* Notify Smart Thumbnail via rtMessage */
#if defined(ENABLE_TEST_HARNESS) && defined(_OBJ_DETECTION_)
    static int notifySmartThumbnail(int32_t pid, uint64_t timestamp, int fileNum, int frameNum, int fps);
#else
    static int notifySmartThumbnail(int32_t pid, uint64_t timestamp);
#endif
    /* Notify Smart Thumbnail rtMessage */
    static int notifySmartThumbnail(char *vaEngineVersion, SmarttnMetadata *smInfo, int motionFlags);
    /* Close rtMessage connection */
    static int rtMessageClose();
    /* Get vai_result structure */
    static vai_result_t* get_vai_structure();
    /* Reset vai_result structure */
    static void reset_vai_results();
    /* Convert iav results */
    static int convert_iav_results(unsigned long long framePTS, vai_result_t * vi_results, iObject *objects, int count);
    /* get Current time */
    static int get_current_time(struct timespec* tstamp);
    /* get smart thumbnail status */
    static bool get_smart_TN_status();
    /* get roi change status */
    static bool get_ROI_status();
    /* set roi change status */
    static void set_ROI_status(bool status);
    /* get doi change status */
    static bool get_DOI_status();
    /* set doi change status */
    static void set_DOI_status(bool status);
#ifdef _OBJ_DETECTION_
#ifdef _ROI_ENABLED_
    static int notifySmartThumbnail(std::vector<float> roicoords);
#endif
    static int notifySmartThumbnail(bool enabled, char * doiBitmapFile, int threshold);
#endif
#ifdef ENABLE_TEST_HARNESS
    static void smtTnOnDeliveryData(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
    static void set_thinterface(THInterface *th);
    static void smtTnOnUpload(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);

#ifdef _OBJ_DETECTION_
    static int notifySmartThumbnail_ClipStatus(int32_t status, char *clip_name);
    static int notifySmartThumbnail_ClipUpload(char *clip_name);
#endif
#endif
 };
#endif

