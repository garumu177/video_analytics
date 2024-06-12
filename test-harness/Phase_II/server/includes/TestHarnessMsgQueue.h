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

#ifndef _TEST_HARNESS_MSG_QUEUE_
#define _TEST_HARNESS_MSG_QUEUE_

/*************************       INCLUDES         *************************/
#include "Common.h"

enum THObjectClass
{
    th_eOC_Unknown = 1,         /**< unknown class */
    th_eOC_Human = 2,           /**< human/person */
    th_eOC_Face = 3,         /**< face */
    th_eOC_Vehicle = 4,         /**< vehicle/car */
    th_eOC_Train = 8,           /**< train */
    th_eOC_Pet = 16,            /**< pet */
    th_eOC_Other = 1073741824,  /**< other class */
    th_eOC_Any = 2147483647     /**< any class */
};

/** Event type */
enum THEventType
{
    th_ePersonFallEvent = 0,           /**< fall */
    th_ePersonDuressEvent = 1,         /**< duress */
    th_eIntrusionEvent = 2,            /**< intrusion */
    th_eObjectEnteredEvent = 3,        /**< object entered the scene */
    th_eObjectExitedEvent = 4,         /**< object exited the scene */
    th_eObjectLeftEvent = 5,           /**< object left */
    th_eObjectRemovedEvent = 6,        /**< object removed */
    th_eObjectCrossedIn = 7,           /**< object crossed the line in 'in' direction */
    th_eObjectCrossedOut = 8,          /**< object crossed the line in 'out' direction */
    th_eLossOfVideo = 9,               /**< loss of video, temporarily not supported */
    th_eLightsOff = 10,                /**< lights turned off */
    th_eLightsOn = 11,                 /**< lights turned on */
    th_eSceneChange = 12,              /**< camera tampered */
    th_eDefocused = 13,                /**< lens defocused, temporarily not supported */
    th_ePersonAbnormalEvent = 14,      /**< object is in abnormal pose */
    th_eObjectSpeedLow = 15,           /**< object has low speed */
    th_eObjectStopped = 16,            /**< object has stopped */
    th_ePersonRunningEvent = 17,       /**< person is in running pose*/
    th_eCrowdDetectedEvent = 18,       /**< crowd is detected */
    th_eDwellEnterEvent = 19,          /**< dwell enter */
    th_eDwellExitEvent = 20,           /**< dwell exit */
    th_eDwellLoiteringEvent = 22,      /**< loitering detected */
    th_eQueueEnteredEvent = 23,        /**< object joined the queue */
    th_eQueueExitedEvent = 24,         /**< object left the queue */
    th_eQueueServicedEvent = 25,       /**< object got his service and left the queue*/
    th_eIntrusionEnteredEvent = 26,    /**< object entered intrusion area */
    th_eIntrusionExitedEvent = 27,     /**< object left intrusion area */
    th_eObjectLost = 28,               /**< object is lost */
    th_eWrongDirection = 29,           /**< object is moving in wrong direction */
    th_eMotionDetected = 30,           /**< motion-based alarm */
    th_eLineCrossed = 31,              /**< line crossed */
    th_eServicedEvent = 32,            /**< person serviced */    th_eUnknownEvent = 255             /**< unknown event */
};

enum THMsgType{
    TH_CLIENT_INFO = 1,
    TH_FILE_INFO   = 2,
    TH_FRAME_INFO  = 3,
    TH_PROCESS_THREAD_EXIT = 4,
    TH_FILE_END    = 5
};

struct THClientInfo{
    std::string mac;
    std::string outputDir;
    AlgoType algType;
    Client clientType;
};

struct THFrameInfo{
    cv::Mat image;
    int frameIndex;
    ProcessedData_t meta_data;
};

struct THFileInfo{
    std::string name;
    unsigned long frameTotal;
    //int fps;
    double fps;
    int rows;
    int cols;
};

struct THMsgQueBuf{
    THMsgType msgType;
    THClientInfo clientInfo;
    THFileInfo fileInfo;
    THFrameInfo frameInfo;
};

#endif

