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

/**
 * @file RdkCVAManager.h
 * @brief Video-Analytics API
 *
 * Video-Analytics API
 */

#ifndef RDKCVAMANAGER_H
#define RDKCVAMANAGER_H

#include "VAStructs.h"
#define RDKC_PROP_MOTION_ENABLED	1600		/**< Enable Motion Detection feature,
                                                           0/1, type: float */
#define RDKC_PROP_OBJECT_ENABLED	1601		/**< Enable Object Detection feature,
                                                           0/1, type: float */
#define RDKC_PROP_HUMAN_ENABLED		1617		/**< Enable Human Detection feature,
                                                           0/1, type: float */
#define RDKC_PROP_SCENE_ENABLED		1618		/**< Enable Tamper Detection feature,
                                                           0/1, type: float */
#define RDKC_PROP_ALGORITHM		1619		/**< Set Video-Analytics algorithm,
							   1=MixtureOfGaussianV2BGS,2=FrameDifferenceBGS,
							   3=PixelBasedAdaptiveSegmenter,4=DPWrenGABGS,
							   5=LBAdaptiveSOM, type: int */
#define RDKC_PROP_SCALE_FACTOR		1620		/**< Set scale factor based on Frame type,
							   1=YUV, 4=ME1, type: int */
#define RDKC_PROP_MOTION_LEVEL		1621		/**< Motion level based on number of pixels under motion
							     type: float */
#define RDKC_PROP_SENSITIVITY		1622		/**< Sensitivity based on Day/Night,
							     type: float */
#define RDKC_PROP_MOTION_LEVEL_RAW	1623		/**< Motion level based on number of pixels under motion
							     type: float */
#define RDKC_PROP_UPSCALE_RESOLUTION	1624		/**< Blob upscaling resolution
							     type: float */
#define RDKC_OD_MAX_NUM			32		/**< Maximum number of objects
							   supported */
#define RDKC_ENABLE			1.0f		/* Enable */
#define RDKC_DISABLE			0.0f		/* Disable */
#ifdef __cplusplus
    #define RDKCVA_EXTERN_C extern "C"
#else
    #define RDKCVA_EXTERN_C
#endif

#define RDKCVA_API RDKCVA_EXTERN_C

/* ---------------------------------------------------------------------------------------------
 *  Recommended sequence of API calls
 *
 *  RdkCVASetAlgorithm() // default GMM
 *  RdkCVAInit()
 *  RdkCVASetProperty()
 *  RdkCVASetImageResolution()
 *  ...
 *  for (;;)
 *  {
 * 	RdkCVAProcessFrame()
 *
 *      RdkCVAGetObjectCount()
 *      RdkCVAGetObject()
 *
 *      RdkCVAGetProperty()
 *
 *      RdkCVAGetEventCount()
 *      RdkCVAGetEvent()
 *  }
 *  RdkCVARelease()
 * --------------------------------------------------------------------------------------------- */


/* ---------------------------------------------------------------------------------------------
 *  enum
 * --------------------------------------------------------------------------------------------- */

enum RdkC_Status{
        VA_SUCCESS = 0,               /* Success */
        VA_FAILURE = -1,              /* Failure */
        RDKC_INVALID = -2,              /* Invalid */
        RDKC_FILE_START = 2,            /* File start */
        RDKC_FILE_END = 3,
        RDKC_MOTION_NOT_DETECTED,       /* No motion */
        RDKC_MOTION_DETECTED,           /* Motion detected */
        RDKC_OBJECT_NOT_DETECTED,       /* No object */
        RDKC_OBJECT_DETECTED,           /* Object detected */
};

enum VA_Algorithm{
        VA_ALG_GMM = 1,         	/* MixtureOfGaussianV2BGS */
        VA_ALG_FD,              	/* FrameDifferenceBGS */
        VA_ALG_PBAS,            	/* PixelBasedAdaptiveSegmenter */
        VA_ALG_DPWREN,          	/* DPWrenGABGS */
        VA_ALG_LBASOM,          	/* LBAdaptiveSOM */
        VA_ALG_IV,              	/* IV */
};

/**
 * Enumeration for motionScore techniques
 */
enum MotionScore_Algorithm{
	MS_MAX_BLOBAREA = 1,
	MS_UNIFORM,
	MS_RANDOM,
	MS_TRACKS,
};

/* ---------------------------------------------------------------------------------------------
 *  Primary API
 * --------------------------------------------------------------------------------------------- */
/* [in] is input parameter
   [out] is output parameter */

/** Get XCV version
@param  [in and out] version_str: xcv algorithm version 
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVAGetXCVVersion(char *version_str, int& versionlength);

/** Set VA algorithm from VA_Algorithm enum values
@param  [in] alg: algorithm to be set
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVASetAlgorithm(int alg);

/** Initialize VA engine
@param  void
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVAInit();

/** Set VA properties
@param	[in] PropID: Property to be set
	[in] val: value to be set for that property
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVASetProperty(int PropID, float val);

/** Get VA properties
@param	[in] PropID: Property to be set
        [out] val: value for that property
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVAGetProperty(int PropID, float *val);

/** Image */
typedef struct _RdkCImage
{
    unsigned char*    data;      /**< pointer to image data */
    unsigned int      size;      /**< image data size in bytes */
    unsigned int      width;     /**< width */
    unsigned int      height;    /**< height */
    unsigned int      step;      /**< step in bytes between image rows */
} RdkCImage;

/** Process frame
@param	[in] *data: Pointer to the frame
	[in] size: size of the frame
	[in] height: height of the frame
	[in] width: width of the frame
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVAProcessFrame( unsigned char *data, int size, int height, int width );

RDKCVA_API int RdkCGetCurrentBlobArea(double blob_threshold, double *currentBlobArea);

RDKCVA_API float RdkCVAGetMotionScore(int mode);

RDKCVA_API int RdkCVAGetObjectBBoxCoords(short *bbox_coords);

#ifdef _OBJ_DETECTION_
RDKCVA_API int RdkCVAGetDeliveryObjectBBox(short *bbox_coords);

RDKCVA_API int RdkCVASetDeliveryUpscaleFactor(float scaleFactor);
#endif
RDKCVA_API int RdkCVASetDOIOverlapThreshold(float threshold);

RDKCVA_API int RdkCVAGetBlobsBBoxCoords(short *bboxs);

#ifdef _ROI_ENABLED_
RDKCVA_API int RdkCVASetROI(std::vector<float> coords);

RDKCVA_API int RdkCVAClearROI();

RDKCVA_API bool RdkCVAIsMotionInsideROI();
#endif

RDKCVA_API bool RdkCVAapplyDOIthreshold(bool enable, char *doi_path, int doi_threshold);

RDKCVA_API bool RdkCVAIsMotionInsideDOI();

/* ---------------------------------------------------------------------------------------------
 *  Object detection API
 * --------------------------------------------------------------------------------------------- */

/** Bounding box */
typedef struct _RdkCBbox
{
    short  m_i_rCol;  /**< right column position */
    short  m_i_lCol;  /**< left column position */
    short  m_i_bRow;  /**< bottom row position */
    short  m_i_tRow;  /**< top row position */
} RdkCBbox;

/** @name Object state */
typedef enum
{
    eRdkCObject_Undefined = -1,  /**< undefined */
    eRdkCObject_Detected  =  0,  /**< object is detected */
    eRdkCObject_Lost      =  1,  /**< object is lost */
    eRdkCObject_New       =  2   /**< object in new state is just detected object */
} eRdkCObjectState;

/** Class */
typedef enum
{
    eRdkCOC_Unknown = 1,         /**< unknown class */
    eRdkCOC_Human = 2,           /**< human/person */
    eRdkCOC_Face = 3,   	     /**< face */
    eRdkCOC_Vehicle = 4,         /**< vehicle/car */
    eRdkCOC_Train = 8,           /**< train */
    eRdkCOC_Pet = 16,            /**< pet */
    eRdkCOC_Other = 1073741824,  /**< other class */
    eRdkCOC_Any = 2147483647     /**< any class */
} eRdkCObjectClass;


/** Object category */
typedef enum
{
    eRdkCUncategorized = 0,  /**< no category */
    eRdkCLeft = 1,           /**< object is left */
    eRdkCRemoved = 2,        /**< object is removed */
    eRdkCSuspected = 3       /**< object is suspected to be left/removed */
} eRdkCObjectCategory;



/** Object */
typedef struct _RdkCObject
{
    unsigned int m_i_ID;            /**< unique object ID */
    eRdkCObjectState m_e_state;         /**< object state DETECTED, TRACKED, LOST ... and so on */
    float        m_f_confidence;    /**< tracking confidence  */
    RdkCBbox        m_u_bBox;          /**< bounding box  */
    eRdkCObjectCategory    m_e_objCategory;        /**< object category: left or removed */
    eRdkCObjectClass       m_e_class;              /**< object class: human, vehicle, train, unknown... */
} RdkCObject;

/**  Get object count
@param	[out] ObjCount: number of objects detected
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVAGetObjectCount(int* ObjCount);

/**  Get object data
@param  [out] refObj: reference object structure
@return VA_SUCCESS on success, VA_FAILURE on failure */
//RDKCVA_API int RdkCVAGetObject(RdkCObject *refObj);
RDKCVA_API int RdkCVAGetObject(iObject *refObj);

/* ---------------------------------------------------------------------------------------------
 *  Motion Events API
 * --------------------------------------------------------------------------------------------- */

/** Date and time */
typedef struct _RdkCDateTime
{
    unsigned int m_i_year;       /**< year - [2007, 2008,...]          */
    unsigned int m_i_month;      /**< months of the year - [1,12]      */
    unsigned int m_i_day;        /**< day of the month - [1,31]        */
    unsigned int m_i_hour;       /**< hours since midnight - [0,23]    */
    unsigned int m_i_minute;     /**< minutes after the hour - [0,59]  */
    unsigned int m_i_second;     /**< seconds after the minute - [0,59]*/
} RdkCDateTime;

/** Event type */
typedef enum
{
    eRdkCPersonFallEvent = 0,           /**< fall */
    eRdkCPersonDuressEvent = 1,         /**< duress */
    eRdkCIntrusionEvent = 2,            /**< intrusion */
    eRdkCObjectEnteredEvent = 3,        /**< object entered the scene */
    eRdkCObjectExitedEvent = 4,         /**< object exited the scene */
    eRdkCObjectLeftEvent = 5,           /**< object left */
    eRdkCObjectRemovedEvent = 6,        /**< object removed */
    eRdkCObjectCrossedIn = 7,           /**< object crossed the line in 'in' direction */
    eRdkCObjectCrossedOut = 8,          /**< object crossed the line in 'out' direction */
    eRdkCLossOfVideo = 9,               /**< loss of video, temporarily not supported */
    eRdkCLightsOff = 10,                /**< lights turned off */
    eRdkCLightsOn = 11,                 /**< lights turned on */
    eRdkCSceneChange = 12,              /**< camera tampered */
    eRdkCDefocused = 13,                /**< lens defocused, temporarily not supported */
    eRdkCPersonAbnormalEvent = 14,      /**< object is in abnormal pose */
    eRdkCObjectSpeedLow = 15,           /**< object has low speed */
    eRdkCObjectStopped = 16,            /**< object has stopped */
    eRdkCPersonRunningEvent = 17,       /**< person is in running pose*/
    eRdkCCrowdDetectedEvent = 18,       /**< crowd is detected */
    eRdkCDwellEnterEvent = 19,          /**< dwell enter */
    eRdkCDwellExitEvent = 20,           /**< dwell exit */
    eRdkCDwellLoiteringEvent = 22,      /**< loitering detected */
    eRdkCQueueEnteredEvent = 23,        /**< object joined the queue */
    eRdkCQueueExitedEvent = 24,         /**< object left the queue */
    eRdkCQueueServicedEvent = 25,       /**< object got his service and left the queue*/
    eRdkCIntrusionEnteredEvent = 26,    /**< object entered intrusion area */
    eRdkCIntrusionExitedEvent = 27,     /**< object left intrusion area */
    eRdkCObjectLost = 28,               /**< object is lost */
    eRdkCWrongDirection = 29,           /**< object is moving in wrong direction */
    eRdkCMotionDetected = 30,           /**< motion-based alarm */
    eRdkCLineCrossed = 31,              /**< line crossed */
    eRdkCServicedEvent = 32,            /**< person serviced */
    eRdkCUnknownEvent = 255             /**< unknown event */
} eRdkCEventType;

/** Event */
typedef struct _RdkCEvent
{
    eRdkCEventType m_e_type;
    int        m_i_reserve1; /**< object ID (if applicable) */
    int        m_i_reserve2; /**< area/line ID (if applicable) */
    int        m_i_reserve3; /**< not used for now */
    int        m_i_reserve4; /**< not used for now */
    RdkCDateTime  m_dt_time;    /**< time when event was created */
} RdkCEvent;

/** Get events count
@param	[out] EvtCount: number of events to be detected
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVAGetEventCount(int* EvtCount);

/** Get event data
@param	[out] refEvent: reference event structure
@return VA_SUCCESS on success, VA_FAILURE on failure */
//RDKCVA_API int RdkCVAGetEvent(RdkCEvent *refEvent);
RDKCVA_API int RdkCVAGetEvent(iEvent *refEvent);

/* ---------------------------------------------------------------------------------------------
 *  Release/Reset API
 * --------------------------------------------------------------------------------------------- */

/** Reset algorithm
@param	void
@return VA_SUCCESS on success, VA_FAILURE on failure */
RDKCVA_API int RdkCVAResetAlgorithm();

/** Release all the occupied memory by VA
@param	void
@return void */
RDKCVA_API void RdkCVARelease();

#endif
