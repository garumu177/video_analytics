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

#ifndef __XCV_H__
#define __XCV_H__

#include "VAStructs.h"

/* Upper limit for number of blob bounding boxes */
#define UPPER_LIMIT_BLOB_BBS 5
#define VA_ENGINE_VERSION 10

class xcvAnalyticsEngine
 {
   public:
    /* Initialize analytics engine */
    virtual int Init() = 0;
    /* Initialization steps to be done only for first time */
    virtual int InitOnce() = 0;
    /* Shut Down analytics engine */
    virtual void Shutdown() = 0;
    /* Process frame */
    virtual void ProcessFrame() = 0;
    /* Get Objects */
    virtual void GetObjects() = 0;
    /* Get count of Objects */
    virtual void GetObjectsCount() = 0;
    /* Get Events */
    virtual void GetEvents() = 0;
    /* Get count of Events */
    virtual void GetEventsCount() = 0;
    /* Set engine property */
    virtual void SetProperty() = 0;
    /* Get engine property */
    virtual void GetProperty() = 0;
    /* Get motion data */
    virtual int GetMotionLevel(float *val) = 0;
    /* Get specific engine property */
    virtual int GetSpecificProperty(int property, float &val) = 0;
    /* Get raw motion */
    virtual void GetRawMotion(unsigned long long framePTS,float &val) = 0;
    /* Read day/night sensitivity value from config file */
    virtual int ReadSensitivityParam() = 0;
    /*Set Sensitivity */
    virtual int SetSensitivity(int curr_day_night_mode) = 0;
    /*Set Day/Night Status mode */
    virtual int SetDayNightMode(int day_night_mode) = 0;
    /*Set Upscale Resolution */
    virtual int SetUpscaleResolution(eRdkCUpScaleResolution_t resolution) = 0;
    /*reset engine*/
    virtual int reset(xcvAnalyticsEngine* engine, int frametype) = 0 ;
    /* get alg index*/
    virtual int GetAlgIndex() = 0;
    /* get scale factor */
    virtual int GetScaleFactor() = 0;
    /* get format */
    virtual int GetFormat() = 0;
    /* get ivengine */
    virtual void* GetIVPointer() = 0;
    /* get VA engine version */
    virtual int GetEngineVersion() = 0;
    /* get motion score */
    virtual int GetMotionScore() = 0;
#ifdef _OBJ_DETECTION_
    /* get Object Box Coords */
    virtual int GetDetectionObjectBBoxCoords() = 0;
    virtual int SetDeliveryUpscaleFactor(float scaleFactor) = 0;
#endif
    virtual int SetDOIOverlapThreshold(float threshold) = 0;
    /* get Object Box Coords */
    virtual int GetObjectBBoxCoords() = 0;
    /* get bounding box coordinates of individual blobs */
    virtual int GetBlobsBBoxCoords() = 0;
#ifdef _ROI_ENABLED_
    /* set ROI Coordinatres */
    virtual int SetROI(std::vector<float> coords) = 0;
    /* Clear ROI Coordinatres */
    virtual int ClearROI() = 0;
    /* Check if motion is inside ROI */
    virtual bool IsMotionInsideROI() = 0;
    /* Get ROI Coordinatres */
    virtual std::vector<float> GetROI() = 0;
    /* Get if ROI set */
    virtual int IsROISet() = 0;
#endif
    /* apply DOI */
    virtual bool applyDOIthreshold(bool enable, char *doi_path, int doi_threshold) = 0;
    /* Check if motion is inside DOI */
    virtual bool IsMotionInsideDOI() = 0;
    /* Get if DOI set */
    virtual int IsDOISet() = 0;

   public:
    int width;
    int height;
    iDateTime dateTime;
    unsigned long long timeStamp;
    unsigned long long framePTS;
    iImage plane0, plane1;
    int objectsCount;
    int eventsCount;
    iObject* objects;
    iEvent* events;
    float motionLevelRaw;
    int frame_type;
    char vaEngineVersion[VA_ENGINE_VERSION+1];
    float motionScore;
    short boundingBoxXOrd;
    short boundingBoxYOrd;
    short boundingBoxHeight;
    short boundingBoxWidth;
#ifdef _OBJ_DETECTION_
    short deliveryBoundingBoxXOrd;
    short deliveryBoundingBoxYOrd;
    short deliveryBoundingBoxHeight;
    short deliveryBoundingBoxWidth;
#endif
    // Bounding boxes for individual motion blobs --> 4 for [x, y, w, h] of rectangle.
    // -1 represents "not a bounding box"
    // Example: [x1, y1, w1, h1, x2, y2, w2, h2, -1, -1, -1, -1, ...]
    //          which means that there were 2 motion blob bounding boxes found
    short blobBoundingBoxCoords[4 * UPPER_LIMIT_BLOB_BBS];
};

/* Types of the class factories to create and destroy Engine */
typedef xcvAnalyticsEngine* CreateEngine_t();
typedef void DestroyEngine_t(xcvAnalyticsEngine*);

#endif
