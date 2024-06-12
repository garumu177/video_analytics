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

#ifndef RDKCTHMANAGER_H
#define RDKCTHMANAGER_H

#define CLIP_DURATION (16)
/** Date and time */
typedef struct _th_iDateTime
{
    unsigned int th_m_i_year;       /**< year - [2007, 2008,...]          */
    unsigned int th_m_i_month;      /**< months of the year - [1,12]      */
    unsigned int th_m_i_day;        /**< day of the month - [1,31]        */
    unsigned int th_m_i_hour;       /**< hours since midnight - [0,23]    */
    unsigned int th_m_i_minute;     /**< minutes after the hour - [0,59]  */
    unsigned int th_m_i_second;     /**< seconds after the minute - [0,59]*/
} th_iDateTime;

/** Image */
typedef struct _th_iImage
{
    unsigned char*    data;      /**< pointer to image data */
    unsigned int      size;      /**< image data size in bytes */
    unsigned int      width;     /**< width */
    unsigned int      height;    /**< height */
    unsigned int      step;      /**< step in bytes between image rows */
} th_iImage;

/** Bounding box */
typedef struct _th_iBbox
{
    short  th_m_i_rCol;  /**< right column position */
    short  th_m_i_lCol;  /**< left column position */
    short  th_m_i_bRow;  /**< bottom row position */
    short  th_m_i_tRow;  /**< top row position */
} th_iBbox;

/** @name Object state */
///@{
typedef enum
{
    th_eObject_Undefined = -1,  /**< undefined */
    th_eObject_Detected  =  0,  /**< object is detected */
    th_eObject_Lost      =  1,  /**< object is lost */
    th_eObject_New       =  2   /**< object in new state is just detected object */
} th_eObjectState;

/** Class */
typedef enum
{
    th_eOC_Unknown = 1,         /**< unknown class */
    th_eOC_Human = 2,           /**< human/person */
    th_eOC_Face = 3,   	     /**< face */
    th_eOC_Vehicle = 4,         /**< vehicle/car */
    th_eOC_Train = 8,           /**< train */
    th_eOC_Pet = 16,            /**< pet */
    th_eOC_Other = 1073741824,  /**< other class */
    th_eOC_Any = 2147483647     /**< any class */
} th_eObjectClass;

/** Object category */
typedef enum
{
    th_eUncategorized = 0,  /**< no category */
    th_eLeft = 1,           /**< object is left */
    th_eRemoved = 2,        /**< object is removed */
    th_eSuspected = 3       /**< object is suspected to be left/removed */
} th_eObjectCategory;

/** Object */
typedef struct _th_iObject
{
    unsigned int 	th_m_i_ID;            /**< unique object ID */
    th_eObjectState     th_m_e_state;         /**< object state DETECTED, TRACKED, LOST ... and so on */
    float               th_m_f_confidence;    /**< tracking confidence  */
    th_iBbox        	th_m_u_bBox;          /**< bounding box  */
    th_eObjectCategory  th_m_e_objCategory;        /**< object category: left or removed */
    th_eObjectClass     th_m_e_class;              /**< object class: human, vehicle, train, unknown... */
} th_iObject;


/** Event type */
typedef enum
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
    th_eServicedEvent = 32,            /**< person serviced */
    th_eUnknownEvent = 255             /**< unknown event */
} th_eEventType;

/** Event */
typedef struct _th_iEvent
{
    th_eEventType th_m_e_type;
    int           th_m_i_reserve1; /**< object ID (if applicable) */
    int           th_m_i_reserve2; /**< area/line ID (if applicable) */
    int           th_m_i_reserve3; /**< not used for now */
    int           th_m_i_reserve4; /**< not used for now */
    th_iDateTime  th_m_dt_time;    /**< time when event was created */
} th_iEvent;

typedef struct _th_deliveryResult
{
    //uint64_t timestamp;
    int32_t fileNum;
    int32_t frameNum;
    /* The bounding boxes of each (n) person detected in the thumbnail */
    std::vector<std::vector<int>> personBBoxes;
    /* The detection score of each (n) person detected in the thumbnail */
    std::vector<double> personScores;
    /* The bounding boxes of each (n) person detected in the thumbnail */
    std::vector<std::vector<int>> nonROIPersonBBoxes;
    /* The detection score of each (n) person detected in the thumbnail */
    std::vector<double> nonROIPersonScores;
    double deliveryScore;
    int32_t maxAugScore;
    double motionTriggeredTime;
    int32_t mpipeProcessedframes;
    double time_taken;
    double time_waited;
} th_deliveryResult;
typedef struct _th_smtnUploadStatus
{
    int status;
} th_smtnUploadStatus;

typedef enum _th_ClipStatus
{
    th_CLIP_GEN_START = 0,
    th_CLIP_GEN_END = 1
} th_ClipStatus;

#endif
