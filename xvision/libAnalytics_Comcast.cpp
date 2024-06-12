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

/*************************       INCLUDES         *************************/

#include <iostream>
#include <chrono>
#include <thread>
#include "libAnalytics_Comcast.h"

//pluginInterface * xcvAnalyticsEngine_Comcast::interface = NULL;
//RdkCPluginFactory * xcvAnalyticsEngine_Comcast::temp_factory = NULL ;
//RdkCVideoCapturer * xcvAnalyticsEngine_Comcast::recorder = NULL;
float xcvAnalyticsEngine_Comcast::day_sen = 0.0;
float xcvAnalyticsEngine_Comcast::night_sen = 0.0;


/** @descripion: Contructor for Comcast Engine
*
*/
xcvAnalyticsEngine_Comcast::xcvAnalyticsEngine_Comcast():curr_day_night_mode(0), upscale_resolution(UPSCALE_RESOLUTION_DEFAULT), roiEnable(false), doiEnable(false)
{
    rdkc_ret = RdkC_Status::VA_FAILURE;
//    interface = new pluginInterface();
//    temp_factory = CreatePluginFactoryInstance(); //creating plugin factory instance
//    recorder = (RdkCVideoCapturer*)temp_factory->CreateVideoCapturer();
}

/** @descripion: Destructor for Comcast Engine
*
*/
xcvAnalyticsEngine_Comcast::~xcvAnalyticsEngine_Comcast()
{

}

/** @descripion: Initializer for Comcast Engine
 *  @return void
 */
int xcvAnalyticsEngine_Comcast::Init()
{
    if( VA_SUCCESS == ReadSensitivityParam() ) {
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): day mode sensitivity: %.2f, night mode sensitivity: %.2f\n",__FUNCTION__, __LINE__,day_sen,night_sen);
    }
    else {
         RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):ReadSensitivityParam fail\n",__FILE__,__LINE__);
	 return XCV_FAILURE;
    }
    return XCV_SUCCESS;
}

/** @descripion: Initializiation to be done only for first time for Comcast Engine
 *  @return void
 */
int xcvAnalyticsEngine_Comcast::InitOnce()
{
    rdkc_ret = static_cast<RdkC_Status>(RdkCVAInit());
    if( VA_SUCCESS != rdkc_ret ) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RdkCVA not initialised successfully!\n",__FUNCTION__, __LINE__);
	return XCV_FAILURE;
    }
    SetProperty();
    if(YUV == frame_type) {
        rdkc_ret = static_cast<RdkC_Status>(RdkCVASetProperty( RDKC_PROP_SCALE_FACTOR, SCALE_FACTOR_YUV));
        if( VA_SUCCESS != rdkc_ret ) {
            RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RDKC_PROP_SCALE_FACTOR failed!\n",__FUNCTION__, __LINE__);
	    return XCV_FAILURE;
        }
    //RdkCVASetImageResolution(width, height);
    }
    else if( ME1 == frame_type) {
        rdkc_ret = static_cast<RdkC_Status>(RdkCVASetProperty( RDKC_PROP_SCALE_FACTOR, SCALE_FACTOR_ME1));
        if( VA_SUCCESS != rdkc_ret ) {
	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RDKC_PROP_SCALE_FACTOR failed!\n",__FUNCTION__, __LINE__);
	    return XCV_FAILURE;
        }
    // RdkCVASetImageResolution(width/4, height/4);
    }

    return XCV_SUCCESS;
}

/** @descripion: Function to get events count
 *  @return void
 */
void xcvAnalyticsEngine_Comcast::GetEventsCount()
{
    rdkc_ret = static_cast<RdkC_Status>(RdkCVAGetEventCount(&eventsCount));
    if( VA_SUCCESS != rdkc_ret ) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RdkCVAGetEventCount failed!\n",__FUNCTION__, __LINE__);
    }
    else {
	RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): RdkCVAGetEventCount success!  Value : %d\n ",__FUNCTION__, __LINE__, eventsCount);
    }
    return;
}

/** @descripion: Function to get events
 *  @return void
 */
void xcvAnalyticsEngine_Comcast::GetEvents()
{
    rdkc_ret = static_cast<RdkC_Status>(RdkCVAGetEvent(events));
    if( VA_SUCCESS != rdkc_ret ) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RdkCVAGetEvent failed!\n",__FUNCTION__, __LINE__);
    }
    return;
}

/** @descripion: Function to get object count
 *  @return void
 */
void xcvAnalyticsEngine_Comcast::GetObjectsCount()
{
    rdkc_ret = static_cast<RdkC_Status>(RdkCVAGetObjectCount(&objectsCount));
    if( VA_SUCCESS != rdkc_ret ) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RdkCVAGetObjectCount failed!\n",__FUNCTION__, __LINE__);
    }
    else {
	RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): RdkCVAGetObjectCount success!  Value : %d\n",__FUNCTION__, __LINE__, objectsCount);
    }
    return;
}

/** @descripion: Function to get objects
*  @return void
*/
void xcvAnalyticsEngine_Comcast::GetObjects()
{
    rdkc_ret = static_cast<RdkC_Status>(RdkCVAGetObject(objects));
    if( VA_SUCCESS != rdkc_ret ) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RdkCVAGetObject failed!\n",__FUNCTION__, __LINE__);
    }
    else {
	RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): RdkCVAGetObject success!\n",__FUNCTION__, __LINE__);
    }
    return;
}

/** @descripion: Function to get specific property from engine
 *  @paremeters:
 *       property [in] property to get value
 *       val [out] val property value
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetSpecificProperty(int property, float &val)
{
	return VA_SUCCESS;

}

/** @descripion: Function to get motion data
 *  @paremeters:
 *        val [out] motion property value
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetMotionLevel(float *value)
{
    int elemCount = 1;
    float val =0.0;
    rdkc_ret = static_cast<RdkC_Status>(RdkCVAGetProperty(RDKC_PROP_MOTION_LEVEL, &val));
    if( VA_SUCCESS != rdkc_ret ) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Get RDKC_PROP_MOTION_LEVEL failed!\n",__FUNCTION__, __LINE__);
    }
    else {
        *value = val;
	RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Get RDKC_PROP_MOTION_LEVEL success! Value : %f\n ",__FUNCTION__, __LINE__, val);
    }
    return (int)rdkc_ret;
}

/** @descripion: Function to get raw motion
 *  @paremeters:
 *       framePTS,val
 *  @return void
 */
void xcvAnalyticsEngine_Comcast::GetRawMotion(unsigned long long framePTS,float &val)
{
    rdkc_ret = static_cast<RdkC_Status>(RdkCVAGetProperty(RDKC_PROP_MOTION_LEVEL_RAW, &val));
    if( VA_SUCCESS != rdkc_ret ) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Get RDKC_PROP_MOTION_LEVEL_RAW failed!\n",__FUNCTION__, __LINE__);
    }
    else {
        RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Get RDKC_PROP_MOTION_LEVEL_RAW success! Value : %f\n ",__FUNCTION__, __LINE__, val);
    }
    return;
}


/** @descripion: Function to get engine properties
 *  @return void
 */
void xcvAnalyticsEngine_Comcast::GetProperty()
{
    return;
}

/** @descripion: Function to set engine properties
 *  @return void
 */
void xcvAnalyticsEngine_Comcast::SetProperty()
{
    float val = 1.0;
    int ret = 0;

    int scene = 1;
    int sen = 8;
    int hd_mode = -1;
    int td_mode = -1;
    FILE *fd = NULL;
    const char* env = NULL;
#ifndef OSI
    fd = fopen(SYSTEM_CONF,"rt");
    PRO_GetInt(SEC_MOTION, SCENE, &scene, fd);
    //   PRO_GetInt(SEC_MOTION, MOTION_SENSITIVITY1, &sen, fd);
    PRO_GetInt(SEC_MOTION, HUMAN_DETECT, &hd_mode, fd);
    PRO_GetInt(SEC_MOTION, SCENE, &td_mode, fd);
    //   PRO_GetInt(SEC_MOTION, FACE_DETECT, &fd_mode, fd);
    if( NULL != fd) {
        fclose(fd);
        fd  = NULL;
    }
#endif
    if( DAY_MODE == curr_day_night_mode ) {
        sen = day_sen;
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","XCV Day mode sensitivity is set to %f.\n",sen);
    }
    else if( NIGHT_MODE == curr_day_night_mode ) {
        sen = night_sen;
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","XCV Night mode sensitivity is set to %f.\n",sen);
    }
    else {
	    RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","Could not read Day-Night mode. Setting sensitivity to default\n");
    	    sen = DEFAULT_XCV_DAY_MODE_SENSITIVITY;
    }

    RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","\nset property: \n");

    val = hd_mode;
    ret = RdkCVASetProperty(RDKC_PROP_HUMAN_ENABLED, val);

    if (VA_SUCCESS != ret) {
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","\t %s(%d): Error: set RDKC_PROP_HUMAN_ENABLED failed: %d\n", __FILE__,__LINE__, ret);
    }
    else {
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","\tSet RDKC_PROP_HUMAN_ENABLED OK.\n");
    }

    val = td_mode;
    ret = RdkCVASetProperty(RDKC_PROP_SCENE_ENABLED, val);

    if(VA_SUCCESS != ret) {
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","\t %s(%d): Error: set RDKC_PROP_SCENE_ENABLED failed: %d\n", __FILE__,__LINE__, ret);
    }
    else {
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","\tSet RDKC_PROP_SCENE_ENABLED OK.\n");
    }

        /* Set the sensitivity */
    val = sen;
    ret = RdkCVASetProperty(RDKC_PROP_SENSITIVITY, val);

    if(VA_SUCCESS != ret)
    {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\t %s(%d): Error: set RDKC_PROP_SENSITIVITY failed %d\n", __FILE__,__LINE__, ret);
    }
    else
    {
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","\tSet RDKC_PROP_SENSITIVITY OK.\n");
    }

    val = upscale_resolution;
    ret = RdkCVASetProperty(RDKC_PROP_UPSCALE_RESOLUTION, val);

    if(VA_SUCCESS != ret)
    {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\t %s(%d): Error: set RDKC_PROP_UPSCALE_RESOLUTION failed %d\n", __FILE__,__LINE__, ret);
    }
    else
    {
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","\tSet RDKC_PROP_UPSCALE_RESOLUTION OK.\n");
    }
    return;

}
/** @descripion: Function to process frame
 *  @return void
 */
void xcvAnalyticsEngine_Comcast::ProcessFrame()
{
    using namespace std::chrono_literals;
    unsigned long long timeStamp = 0;
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): plane0: data=0x%X, size=%u, width:%u, height:%u, step:%u\n",  __FILE__, __LINE__, plane0.data, plane0.size, plane0.width, plane0.height, plane0.step);
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): plane1: data=0x%X, size=%u, width:%u, height:%u, step:%u\n",  __FILE__, __LINE__, plane1.data, plane1.size, plane1.width, plane1.height, plane1.step);
    //xcvInterface::get_iDateTime(&dateTime);

    // initialize timeStamp with current frame timestamp in ms
//    timeStamp = iavInterfaceAPI::getmsec();
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): current timestamp: %u ms\n", __FILE__, __LINE__, timeStamp);

    // process frame
    // plane0 is Y value, plane1 is interleaved UV
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): do detection processing...\n" ,__FILE__, __LINE__);

    rdkc_ret = static_cast<RdkC_Status>(RdkCVAProcessFrame( plane0.data, plane0.size, plane0.height, plane0.width));
    if( VA_SUCCESS != rdkc_ret ) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RdkCVAProcessFrame failed!\n",__FUNCTION__, __LINE__);
    }
    else {
	RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): RdkCVAProcessFrame success!\n",__FUNCTION__, __LINE__);
    }
    return;
}

/** @descripion: Function to shut down comcast analytics engine
 *  @return void
 */
void xcvAnalyticsEngine_Comcast::Shutdown()
{
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):Engine Shutdown\n",__FUNCTION__, __LINE__);
    RdkCVARelease();
    return;
}

/** @description: Read day/night sensitivity value from config file
 *  @param: void
 *  @return: sensitivity value
 */
int xcvAnalyticsEngine_Comcast::ReadSensitivityParam()
{
    char* configParam = NULL;
    char config_param[CONFIG_PARAM_MAX_LENGTH];
    memset(config_param, 0, CONFIG_PARAM_MAX_LENGTH);

    /* Read Day mode sensitivity for XCV from configuration file */
    if (VA_SUCCESS != rdkc_get_user_setting(XCV_DAY_SENSITIVITY, config_param)) {
	configParam = (char*)rdkc_envGet(XCV_DAY_SENSITIVITY);
    }
    else {
	configParam = config_param;
    }

    if( NULL == configParam ) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: Could not read Day mode sensitivity for XCV from config file. Setting it to default %d\n",DEFAULT_XCV_DAY_MODE_SENSITIVITY);
	day_sen = DEFAULT_XCV_DAY_MODE_SENSITIVITY;
    }
    else {
	day_sen = atof(configParam);
	if( day_sen <= XCV_MIN_SENSITIVITY || day_sen > XCV_MAX_SENSITIVITY ) {
	    RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: Invalid Day mode sensitivity for XCV. Setting it to default %d\n",DEFAULT_XCV_DAY_MODE_SENSITIVITY);
	    day_sen = DEFAULT_XCV_DAY_MODE_SENSITIVITY;
	}
    }
    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","XCV Day mode sensitivity is set to %.2f.\n",day_sen);

    /* Read Night mode sensitivity for XCV from configuration file */
    if (VA_SUCCESS != rdkc_get_user_setting(XCV_NIGHT_SENSITIVITY, config_param)) {
	configParam = (char*)rdkc_envGet(XCV_NIGHT_SENSITIVITY);
    }
    else {
	configParam = config_param;
    }

    if( NULL == configParam ) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: Could not read Night mode sensitivity for XCV from config file. Setting it to default %d\n",DEFAULT_XCV_NIGHT_MODE_SENSITIVITY);
	night_sen = DEFAULT_XCV_NIGHT_MODE_SENSITIVITY;
    }
    else {
	night_sen = atof(configParam);
	if( night_sen <= XCV_MIN_SENSITIVITY || night_sen > XCV_MAX_SENSITIVITY ) {
	    RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: Invalid Night mode sensitivity for XCV. Setting it to default %d\n",DEFAULT_XCV_NIGHT_MODE_SENSITIVITY);
	    night_sen = DEFAULT_XCV_NIGHT_MODE_SENSITIVITY;
	}
    }
    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","XCV Night mode sensitivity is set to %.2f.\n",night_sen);

    return VA_SUCCESS;
}

/** @description: set XCV engine sensitivity according to Day/Night mode
 *  @param[in] curr_day_night_mode : current camera mode (Day/Night)
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int xcvAnalyticsEngine_Comcast::SetSensitivity(int curr_day_night_mode)
{
    float sen = 0.0;
    if( DAY_MODE == curr_day_night_mode ) {
	sen = day_sen;
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","XCV Day mode sensitivity is set to %f.\n",sen);
    }
    else if( NIGHT_MODE == curr_day_night_mode ) {
	sen = night_sen;
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","XCV Night mode sensitivity is set to %f.\n",sen);
    }

    /* Set the sensitivity */
    if( VA_SUCCESS != RdkCVASetProperty(RDKC_PROP_SENSITIVITY, sen) ) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: set RDKC_PROP_SENSITIVITY failed. \n");
	return VA_FAILURE;
    }
    else {
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","\tSet RDKC_PROP_SENSITIVITY OK.\n");
    }
    return VA_SUCCESS;

}

/** @description: set upscale resolution
 *  @param: day_night_mode : int
 */
int xcvAnalyticsEngine_Comcast::SetUpscaleResolution(eRdkCUpScaleResolution_t resolution)
{
    upscale_resolution = resolution;
    return VA_SUCCESS;
}

/** @description: set Day/Night mode status
 *  @param: day_night_mode : int
 */
int xcvAnalyticsEngine_Comcast::SetDayNightMode(int day_night_mode)
{
    curr_day_night_mode = day_night_mode;
    return VA_SUCCESS;
}

/** @descripion: Function to reset Engine 
 *  @param: engine - xcvAnalyticsEngine pointer
 *  frametype - int 
 */
int xcvAnalyticsEngine_Comcast::reset(xcvAnalyticsEngine* engine, int frametype)
{
    if(NULL == engine) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): xcvAnalyticsEngine is NULL\n",__FUNCTION__,__LINE__);
        return XCV_FAILURE;
    }

    engine->width = 0;
    engine->height = 0;
    engine->framePTS = 0;
    engine->timeStamp = 0;
    engine->objectsCount = 0;
    engine->eventsCount = 0;
    engine->objects = NULL;
    engine->events = NULL;
    engine->motionLevelRaw = 0;
    engine->frame_type = frametype;
    return XCV_SUCCESS;
}

/** @descripion: Function to get Alg Index
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetAlgIndex()
{
    return RDKCVA; 
}
/** @descripion: Function to get ScaleFactor
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetScaleFactor()
{
    return XCV_SUCCESS;
}

/** @descripion: Function to get Format
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetFormat()
{
    return XCV_SUCCESS;
}

/** @descripion: Function to get Iv engine pointer
 *  @return void *
 */
void* xcvAnalyticsEngine_Comcast::GetIVPointer()
{
    return NULL;
}

/** @description: Get VA engine version
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetEngineVersion()
{
    int versionlength = VA_ENGINE_VERSION+1;
    if ( VA_SUCCESS != RdkCVAGetXCVVersion(vaEngineVersion,versionlength)) {
	 RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError retriving VA engine version\n",__FILE__,__LINE__);
	 return VA_FAILURE;
    }
    return VA_SUCCESS;
}

/** @description: Get motion score
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetMotionScore()
{
    motionScore = RdkCVAGetMotionScore(MS_MAX_BLOBAREA);
    return VA_SUCCESS;
}

#ifdef _OBJ_DETECTION_
/** @description: Get Object Box Coords
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetDetectionObjectBBoxCoords()
{
    short bBoxCoord[4] = {0};

    if( VA_SUCCESS != RdkCVAGetDeliveryObjectBBox(bBoxCoord)) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError retriving bounding box co ordinate\n",__FILE__,__LINE__);
        return VA_FAILURE;
    }

    deliveryBoundingBoxXOrd = bBoxCoord[0];
    deliveryBoundingBoxYOrd = bBoxCoord[1];
    deliveryBoundingBoxWidth = bBoxCoord[2];
    deliveryBoundingBoxHeight = bBoxCoord[3];

    return VA_SUCCESS;
}

int xcvAnalyticsEngine_Comcast::SetDeliveryUpscaleFactor(float scaleFactor)
{

    if( VA_SUCCESS != RdkCVASetDeliveryUpscaleFactor(scaleFactor)) {
       RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError setting Delivery upscale factor\n",__FILE__,__LINE__);
        return VA_FAILURE;
    }

    return VA_SUCCESS;
}

#endif

int xcvAnalyticsEngine_Comcast::SetDOIOverlapThreshold(float threshold)
{

    if( VA_SUCCESS != RdkCVASetDOIOverlapThreshold(threshold)) {
       RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError setting DOI overlap threshold\n",__FILE__,__LINE__);
        return VA_FAILURE;
    }

    return VA_SUCCESS;
}
/** @description: Get Object Box Coords
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetObjectBBoxCoords()
{
    short bBoxCoord[4] = {0};

    if( VA_SUCCESS != RdkCVAGetObjectBBoxCoords(bBoxCoord)) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError retriving bounding box co ordinate\n",__FILE__,__LINE__);
        return VA_FAILURE;
    }

    boundingBoxXOrd = bBoxCoord[0];
    boundingBoxYOrd = bBoxCoord[1];
    boundingBoxWidth = bBoxCoord[2];
    boundingBoxHeight = bBoxCoord[3];

    return VA_SUCCESS;
}

/** @description: Get bounding box coordinates of individual blobs.
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::GetBlobsBBoxCoords()
{

    if( VA_SUCCESS != RdkCVAGetBlobsBBoxCoords(blobBoundingBoxCoords)) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError retrieving individual blob bounding box coordinates\n",__FILE__,__LINE__);
        return VA_FAILURE;
    }

    return VA_SUCCESS;
}

#ifdef _ROI_ENABLED_

/** @description: Set ROI
 *  @param: coords - coordinates list
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::SetROI(std::vector<float> coords)
{
    m_coords.clear();
    if( VA_SUCCESS != RdkCVASetROI(coords)) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError setting ROI\n",__FILE__,__LINE__);
        return VA_FAILURE;
    }
    m_coords = coords;
    roiEnable = true;
    return VA_SUCCESS;
}

/** @description: Get ROI
 *  @param: void
 *  @return: Fetch the coordinates that were set to the engine
 *      returns empty vector if coordinates were not set
 */
std::vector<float> xcvAnalyticsEngine_Comcast::GetROI()
{
    return m_coords;
}

/** @description: Clear ROI
 *  @param:
 *  @return int
 */
int xcvAnalyticsEngine_Comcast::ClearROI()
{
    if( VA_SUCCESS != RdkCVAClearROI()) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError clearing ROI\n",__FILE__,__LINE__);
        return VA_FAILURE;
    }
    m_coords.clear();
    roiEnable = false;
    return VA_SUCCESS;
}

/** @description: Check if motion is inside ROI
 *  @param:
 *  @return bool
 */
bool xcvAnalyticsEngine_Comcast::IsMotionInsideROI()
{
    return RdkCVAIsMotionInsideROI();
}

/** @description: Check if ROI set
 *  @param:
 *  @return 1 if set, else 0
 */
int xcvAnalyticsEngine_Comcast::IsROISet()
{
    return (roiEnable ? 1 : 0);
}
#endif

bool xcvAnalyticsEngine_Comcast::applyDOIthreshold(bool enable, char *doi_path, int doi_threshold) {
    doiEnable = false;
    if(VA_FAILURE != RdkCVAapplyDOIthreshold(enable, doi_path, doi_threshold)) {
        if(enable) {
            doiEnable = true;
        }
	return true;
    } else {
        return false;
    }
}

/** @description: Check if motion is inside DOI
 *  @param:
 *  @return bool
 */
bool xcvAnalyticsEngine_Comcast::IsMotionInsideDOI()
{
    return RdkCVAIsMotionInsideDOI();
}

/** @description: Check if DOI set
 *  @param:
 *  @return 1 if set, else 0
 */
int xcvAnalyticsEngine_Comcast::IsDOISet()
{
    return (doiEnable ? 1 : 0);
}

/** @descripion: Function to create comcast analytics engine
 *  @return xcvAnalyticsEngine_Comcast object
 */
xcvAnalyticsEngine* CreateEngine()
{
    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):\tCreate Engine\n",__FILE__,__LINE__);
    return new xcvAnalyticsEngine_Comcast();
}

/** @descripion: Function to destroy comcast analytics engine
 *  @return void
 */
void DestroyEngine(xcvAnalyticsEngine* e)
{
    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):\tDestroy Engine\n",__FILE__,__LINE__);
    if(NULL != e) {
        delete e;
    }
    return;
}
