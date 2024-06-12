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

#ifndef _LIBANALYTICS_INTELLIVISION_H_
#define _LIBANALYTICS_INTELLIVISION_H_

#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include "xcv.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "PRO_file.h"
#ifdef __cplusplus
}
#endif

#define IV 					6
#define DEFAULT_IV_DAY_MODE_SENSITIVITY         8       /* Default value for IV day sensitivity */
#define DEFAULT_IV_NIGHT_MODE_SENSITIVITY       8       /* Default value for IV night  sensitivity */
#define IV_MIN_SENSITIVITY                      0       /* Minimum value for IV sensitivity */
#define IV_MAX_SENSITIVITY                      10      /* Maximum value for IV sensitivity */
#define SCENE1                                  1       /*scene 1*/
#define SCENE2                                  2       /*scene 2*/
#define LICSIZE                                 32

class xcvAnalyticsEngine_Intellivision : public xcvAnalyticsEngine
{
   public:
    /* Constructor */
	xcvAnalyticsEngine_Intellivision();
	/* Destructor */
	~xcvAnalyticsEngine_Intellivision();
	/* Initialize analytics engine */
	virtual int Init();
	/* Initialization steps to be done only for first time */
	virtual int InitOnce();
	/* Shut Down analytics engine */
	virtual void Shutdown();
	/* Process frame */
	virtual void ProcessFrame();
	/* Get Objects */
	virtual void GetObjects();
	/* Get count of Objects */
	virtual void GetObjectsCount();
	/* Get Events */
	virtual void GetEvents();
	/* Get count of Events */
	virtual void GetEventsCount();
	/* Set engine property */
	virtual void SetProperty();
	/* Get engine property */
	virtual void GetProperty();
	/* Get motion data */
	virtual int GetMotionLevel(float *val);
	 /* Get specific engine property */
	virtual int GetSpecificProperty(int property, float &val);
	 /* Get raw motion */
	virtual void GetRawMotion(unsigned long long framePTS,float &val);
	 /* Read day/night sensitivity value from config file */
	virtual int ReadSensitivityParam();
	 /* Set Sensitivity */
	virtual int SetSensitivity(int curr_day_night_mode);
	 /*read Day/Night status mode */
//        virtual int read_DN_mode(PLUGIN_DayNightStatus &day_night_status);
	/* set Day Night Mode */
	virtual int SetDayNightMode(int day_night_mode);
         /* reset engine */
        virtual int reset(xcvAnalyticsEngine* engine, int frametype);
         /*get alg index*/
	virtual int GetALgIndex();
	 /* get scale factor */
        virtual int GetScaleFactor();
         /* get format */
        virtual int GetFormat();
	 /* get Iv pointer */
	virtual void* GetIVPointer();
	/* get VA engine version */
    	virtual int GetEngineVersion();
    	/* get motion score */
    	virtual int GetMotionScore();
#ifdef _OBJ_DETECTION_
       /* get Object Box Coords */
       virtual int GetDetectionObjectBBoxCoords();
       virtual int SetDeliveryUpscaleFactor(float scaleFactor);
#endif
	int SetDOIOverlapThreshold(float threshold);
    	/* get Object Box Coords */
    	virtual int GetObjectBBoxCoords();
	/* Set Upscale resolution */
	virtual int SetUpscaleResolution(eRdkCUpScaleResolution_t resolution);
#ifdef _ROI_ENABLED_
        /* set ROI Coordinatres */
        virtual int SetROI(std::vector<float> coords);
        /* Clear ROI Coordinatres */
        virtual int ClearROI();
	/* Check if motion is inside ROI */
	virtual bool IsMotionInsideROI();
        /* Get if ROI set */
        virtual int IsROISet();
#endif
        /* apply DOI */
        virtual bool applyDOIthreshold(bool enable, char *doi_path, int doi_threshold);
        /* Check if motion is inside DOI */
        virtual bool IsMotionInsideDOI();
        /* Get if DOI set */
        virtual int IsDOISet();

   private:
	static void *ivengine;
	int format;   // specify input video stream format
	int scaleFactor;
	char license[LICSIZE+1];// = "jp394jzMOE54nf7oa7uZLEN587b5";
	iLicenseInfo info;
//	static pluginInterface *interface;
//	static RdkCPluginFactory *temp_factory;
//	static RdkCVideoCapturer *recorder;
	//Sensitivity parameters
	static float day_sen;
	static float night_sen;
	int curr_day_night_mode;

};

/* Types of the class factories to create and destroy Engine */
extern "C" xcvAnalyticsEngine* CreateEngine();
extern "C" void DestroyEngine(xcvAnalyticsEngine* e);

#endif
