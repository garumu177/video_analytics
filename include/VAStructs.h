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

#ifndef IV_ENGINE_MANAGER_H
#define IV_ENGINE_MANAGER_H

#include <vector>

#define RDKCVA                                  5       /* VA Enagine : RDKC */
#define YUV                                     1       /* Frame type : yuv */
#define ME1                                     2       /* Frame type : me */

#define IV_BAD_PARAM                            -1

#define I_PATH_LENGTH                		30      /**< trace path length */
#define I_COLOR_SIG_LENGTH           		36      /**< color signature bin number */
#define I_FEATURE_POINTS_MAX_NUMBER  		256     /**< maximal number of feature points */

#define IV_NUM_CLASSES 				(2)     /**< number of classes which have confidences
                                              		 in object metadata */

/** Date and time */
typedef struct _iDateTime
{
    unsigned int m_i_year;       /**< year - [2007, 2008,...]          */
    unsigned int m_i_month;      /**< months of the year - [1,12]      */
    unsigned int m_i_day;        /**< day of the month - [1,31]        */
    unsigned int m_i_hour;       /**< hours since midnight - [0,23]    */
    unsigned int m_i_minute;     /**< minutes after the hour - [0,59]  */
    unsigned int m_i_second;     /**< seconds after the minute - [0,59]*/
} iDateTime;


/** Image */
typedef struct _iImage
{
    unsigned char*    data;      /**< pointer to image data */
    unsigned int      size;      /**< image data size in bytes */
    unsigned int      width;     /**< width */
    unsigned int      height;    /**< height */
    unsigned int      step;      /**< step in bytes between image rows */
} iImage;

/** @name Object state */
///@{
typedef enum
{
    eObject_Undefined = -1,  /**< undefined */
    eObject_Detected  =  0,  /**< object is detected */
    eObject_Lost      =  1,  /**< object is lost */
    eObject_New       =  2   /**< object in new state is just detected object */
} eObjectState;
///@}

typedef struct _iBbox
{
    short  m_i_rCol;  /**< right column position */
    short  m_i_lCol;  /**< left column position */
    short  m_i_bRow;  /**< bottom row position */
    short  m_i_tRow;  /**< top row position */
} iBbox;

/** Object color */
typedef enum {
    eUnknownColor = 0,
    eBlack,  eDarkBlue,  eNavy,  eMediumBlue,  eBlue,  eDarkGreen,  ePrussianBlue,
    eLakeBlue, eMediumGreen,  eEmpireGreen, eTeal, eSkyBlue, eLightBlue, eGreen,
    eLime, eSpringGreen, eAqua, eDarkRed, eDioxazinePurple, eIndigo,eSeaBlue,
    eMaroon, eReddishViolet, ePurple, eBlueViolet, eOliveBrown, eWitheredRose,
    eLightOlive, eBrightOlive, eGray, eSlateBlue, eLightSkyBlue, eLawnGreen,
    eLightLime, eLightCyan, eSilver, eRed, eDeepPink, eMagenta, eOrange, eOrangeYellow,
    eMallowPink, ePerfectPink, eLightPink, eYellow, eLightYellow, eWhite, eeDioxazinePurple2,
    eTan, eSeaGreen, eDarkMagenta
} eObjectColor;


/** Color */
typedef struct _iColor
{
    unsigned char r;  /**< red component*/
    unsigned char g;  /**< green component*/
    unsigned char b;  /**< blue component*/
} iColor;


/** Color bin */
typedef struct _iEnhancedColorBin
{
    unsigned int i_weight;         /**< weight of color segment normalized on 1000 */
    unsigned short i_meanX;        /**< X-coordinate of the center of mass of color segment
                                        in pixels of internal resolution */
    unsigned short i_meanY;        /**< Y-coordinate of the center of mass of color segment
                                        in pixels of internal resolution */
    unsigned short i_dispersionX;  /**< half-width of color segment in pixels of internal resolution */
    unsigned short i_dispersionY;  /**< half-height of color segment in pixels of internal resolution */
} iEnhancedColorBin;


/** Point */
typedef struct _iPoint
{
    int x;  /**< x-coordinate */
    int y;  /**< y0coordinate */
} iPoint;

/** Pose */
typedef enum
{
    eObjectPoseUnavailable = 0,  /**< pose unavailable */
    eObjectPoseGeneric     = 1,  /**< generic */
    eObjectPoseFall        = 2,  /**< fall */
    eObjectPoseDuress      = 3,  /**< duress */
    eObjectPoseAbnormal    = 4,  /**< abnormal */
    eObjectPoseRunning     = 5   /**< running */
} eObjectPose;

/** Class */
typedef enum
{
    eOC_Unknown = 1,         /**< unknown class */
    eOC_Human = 2,           /**< human/person */
    eOC_Vehicle = 4,         /**< vehicle/car */
    eOC_Train = 8,           /**< train */
    eOC_Pet = 16,            /**< pet */
    eOC_Other = 1073741824,  /**< other class */
    eOC_Any = 2147483647     /**< any class */
} eObjectClass;


/** Object category */
typedef enum
{
    eUncategorized = 0,  /**< no category */
    eLeft = 1,           /**< object is left */
    eRemoved = 2,        /**< object is removed */
    eSuspected = 3       /**< object is suspected to be left/removed */
} eObjectCategory;


/** Vehicle speed state */
typedef enum _eVehicleSpeedState
{
    eVS_Undefined = 0,   /**< Undefined */
    eVS_SpeedHigh = 1,   /**< High Speed */
    eVS_SpeedNormal = 2, /**< Normal Speed */
    eVS_SpeedLow = 3,    /**< Low Speed */
    eVS_Stopped = 4      /**< Stopped */
} eVehicleSpeedState;

/**
 * Enumeration for scaling Resolution
 */
typedef enum _eRdkCUpScaleResolution{
	UPSCALE_RESOLUTION_DEFAULT = 1,
	UPSCALE_RESOLUTION_FIRST = 1,
	UPSCALE_RESOLUTION_1280_720 = 1,
	UPSCALE_RESOLUTION_1280_960,
        UPSCALE_RESOLUTION_640_480,
	UPSCALE_RESOLUTION_LAST = UPSCALE_RESOLUTION_640_480
} eRdkCUpScaleResolution_t;

/** Face */
typedef struct _iFace
{
        unsigned char m_b_hasID;         /**< 0- face does not have ID, 1 - does */
        unsigned int  m_i_ID;            /**< face ID */
        iBbox         m_u_bBox;          /**< bounding box  */
} iFace;


/** Event type */
typedef enum
{
    ePersonFallEvent = 0,           /**< fall */
    ePersonDuressEvent = 1,         /**< duress */
    eIntrusionEvent = 2,            /**< intrusion */
    eObjectEnteredEvent = 3,        /**< object entered the scene */
    eObjectExitedEvent = 4,         /**< object exited the scene */
    eObjectLeftEvent = 5,           /**< object left */
    eObjectRemovedEvent = 6,        /**< object removed */
    eObjectCrossedIn = 7,           /**< object crossed the line in 'in' direction */
    eObjectCrossedOut = 8,          /**< object crossed the line in 'out' direction */
    eLossOfVideo = 9,               /**< loss of video, temporarily not supported */
    eLightsOff = 10,                /**< lights turned off */
    eLightsOn = 11,                 /**< lights turned on */
    eSceneChange = 12,              /**< camera tampered */
    eDefocused = 13,                /**< lens defocused, temporarily not supported */
    ePersonAbnormalEvent = 14,      /**< object is in abnormal pose */
    eObjectSpeedLow = 15,           /**< object has low speed */
    eObjectStopped = 16,            /**< object has stopped */
    ePersonRunningEvent = 17,       /**< person is in running pose*/
    eCrowdDetectedEvent = 18,       /**< crowd is detected */
    eDwellEnterEvent = 19,          /**< dwell enter */
    eDwellExitEvent = 20,           /**< dwell exit */
    eDwellLoiteringEvent = 22,      /**< loitering detected */
    eQueueEnteredEvent = 23,        /**< object joined the queue */
    eQueueExitedEvent = 24,         /**< object left the queue */
    eQueueServicedEvent = 25,       /**< object got his service and left the queue*/
    eIntrusionEnteredEvent = 26,    /**< object entered intrusion area */
    eIntrusionExitedEvent = 27,     /**< object left intrusion area */
    eObjectLost = 28,               /**< object is lost */
    eWrongDirection = 29,           /**< object is moving in wrong direction */
    eMotionDetected = 30,           /**< motion-based alarm */
    eLineCrossed = 31,              /**< line crossed */
    eServicedEvent = 32,            /**< person serviced */
    eUnknownEvent = 255             /**< unknown event */
} eEventType;


/** Event */
typedef struct _iEvent
{
    eEventType m_e_type;
    int        m_i_reserve1; /**< object ID (if applicable) */
    int        m_i_reserve2; /**< area/line ID (if applicable) */
    int        m_i_reserve3; /**< not used for now */
    int        m_i_reserve4; /**< not used for now */
    iDateTime  m_dt_time;    /**< time when event was created */
} iEvent;

typedef struct _iObject
{
    unsigned int m_i_ID;            /**< unique object ID */
    eObjectState m_e_state;         /**< object state DETECTED, TRACKED, LOST ... and so on */
    float        m_f_confidence;    /**< tracking confidence  */
    iBbox        m_u_bBox;          /**< bounding box  */

    eObjectColor      m_e_color;                                 /**< one of colors in palette */
    iColor            m_u_primaryRGB;                            /**< primary RGB */
    iColor            m_u_secondaryRGB;                          /**< secondary RGB */
    unsigned char     m_pu_colorSig[I_COLOR_SIG_LENGTH];         /**< signature of object colors
                                                                      normalized to 100 */
    iEnhancedColorBin m_pu_enhancedColorSig[I_COLOR_SIG_LENGTH]; /**< signature of object colors
                                                                      with color segment locations */

    float   m_f_recentSpeed;        /**< Speed in pixels per second over the recent object path */
    float   m_f_averageSpeed;       /**< Average speed in pixels per second */

    float   m_f_recentDirection;    /**< Moving direction in degrees over the recent object path.
                                         The range is 0..360: 0 - to the right, 90 - to the bottom,
                                         180 - to the left, 270 - to the top  */
    float   m_f_averageDirection;   /**< Moving direction in degrees between entry and current locations.
                                         The range is 0..360: 0 - to the right, 90 - to the bottom,
                                         180 - to the left, 270 - to the top  */

    iPoint  m_pu_lastPathPoints[I_PATH_LENGTH]; /**< points of path */
    iBbox   m_pu_lastPathRects[I_PATH_LENGTH];  /**< rects of path */
    int     m_i_lastPathPointsNum;              /**< number of points of path */
    int     m_i_distance;           /**< distance travelled */

    iPoint  m_u_entryPoint;         /**< point where object was detected for the first time */
    iPoint  m_u_massCentroid;       /**< mass centroid */
    int     m_i_objectArea;         /**< number of pixels of an object at FG mask */

    iPoint  m_pu_featurePoints[I_FEATURE_POINTS_MAX_NUMBER]; /**< feature points of object */
    int     m_i_featurePointsNumber;                         /**< feature points number */

    iDateTime          m_dt_entryTime;         /**< entry date & time */
    float              m_f_lifeTime;           /**< time of object life in seconds */
    eObjectCategory    m_e_objCategory;        /**< object category: left or removed */
    eObjectPose        m_e_pose;               /**< object pose */
    eObjectClass       m_e_class;              /**< object class: human, vehicle, train, unknown... */
        float              m_pf_classConfidences[IV_NUM_CLASSES]; /**< array of confidences of different classes */
    eVehicleSpeedState m_e_vehicleSpeedState;  /**< vehicle speed state */
} iObject;

#endif
