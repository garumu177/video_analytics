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

#ifndef _LIBANALYTICS_COMCAST_H_
#define _LIBANALYTICS_COMCAST_H_

#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include "xcv.h"
#include "dev_config.h"
#include "va_defines.h"
#include "rdk_debug.h"
#ifndef OSI
#include "conf_sec.h"
#endif
#include "RdkCVAManager.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifndef OSI
#include "PRO_file.h"
#endif
#ifdef __cplusplus
}
#endif

#define DEFAULT_XCV_DAY_MODE_SENSITIVITY        1       /* Default value for xcv day sensitivity */
#define DEFAULT_XCV_NIGHT_MODE_SENSITIVITY      1       /* Default value for xcv night sensitivity */
#define XCV_MIN_SENSITIVITY                     0       /* Minimum value for XCV sensitivity */
#define XCV_MAX_SENSITIVITY                     2       /* Maximum value for XCV sensitivity */
#define RDKCVA                                  5       /* VA Enagine : RDKC */
#define YUV                                     1       /* Frame type : yuv */
#define ME1                                     2       /* Frame type : me */

#define IV_BAD_PARAM                            -1

#define I_PATH_LENGTH                           30      /**< trace path length */
#define I_COLOR_SIG_LENGTH                      36      /**< color signature bin number */
#define I_FEATURE_POINTS_MAX_NUMBER             256     /**< maximal number of feature points */

#define IV_NUM_CLASSES                          (2)     /**< number of classes which have confidences
                                                         in object metadata */

class xcvAnalyticsEngine_Comcast : public xcvAnalyticsEngine
{
    public:
      /* Constuctor */
      xcvAnalyticsEngine_Comcast();
      /* Destructor */
      ~xcvAnalyticsEngine_Comcast();
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
      /* ReadSensitivity */
      virtual int ReadSensitivityParam();
      /* Set Sensitivity */
      virtual int SetSensitivity(int curr_day_night_mode);
      /*read Day/Night Status mode */
//      virtual int read_DN_mode(PLUGIN_DayNightStatus &day_night_status);
      /* set Day Night Mode */
      virtual int SetDayNightMode(int day_night_mode);
      /* set Upscale Resolution */
      virtual int SetUpscaleResolution(eRdkCUpScaleResolution_t resolution);
      /* reset engine */
      virtual int reset(xcvAnalyticsEngine* engine, int frametype);
      /*get alg index*/
      virtual int GetAlgIndex();
      /* get scale factor */
      virtual int GetScaleFactor();
      /* get format */
      virtual int GetFormat();
      /* get ivengine pointer */
      virtual void * GetIVPointer();
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
      /* get bounding box coordinates of individual blobs */
      virtual int GetBlobsBBoxCoords();
#ifdef _ROI_ENABLED_
      /* set ROI Coordinatres */
      virtual int SetROI(std::vector<float> coords);
      /* Clear ROI Coordinatres */
      virtual int ClearROI();
      /* Check if motion is inside ROI */
      virtual bool IsMotionInsideROI();
      /* Get ROI Coordinatres */
      virtual std::vector<float> GetROI();
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
      enum RdkC_Status rdkc_ret;
      //static pluginInterface *interface;
      //static RdkCPluginFactory *temp_factory;
      //static RdkCVideoCapturer *recorder;
      //Sensitivity parameters
      static float day_sen;
      static float night_sen;
      int curr_day_night_mode;
      eRdkCUpScaleResolution_t upscale_resolution;
#ifdef _ROI_ENABLED_
      std::vector<float> m_coords;
      int roiEnable;
#endif
      int doiEnable;
};

/* Types of the class factories to create and destroy Engine */
extern "C" xcvAnalyticsEngine* CreateEngine();
extern "C" void DestroyEngine(xcvAnalyticsEngine* e);

#endif
