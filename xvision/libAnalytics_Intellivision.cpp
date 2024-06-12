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
#include "libAnalytics_Intellivision.h"

void* xcvAnalyticsEngine_Intellivision::ivengine = NULL;
//pluginInterface* xcvAnalyticsEngine_Intellivision::interface = NULL;
//RdkCPluginFactory* xcvAnalyticsEngine_Intellivision::temp_factory = NULL;
//RdkCVideoCapturer* xcvAnalyticsEngine_Intellivision::recorder = NULL;
float xcvAnalyticsEngine_Intellivision::day_sen = 0.0;
float xcvAnalyticsEngine_Intellivision::night_sen = 0.0;

/** @descripion: Contructor for Intellivision Engine
 *
 */
xcvAnalyticsEngine_Intellivision::xcvAnalyticsEngine_Intellivision():curr_day_night_mode(0)
{
    format = eICS_YUV420SP;   // specify input video stream format
    scaleFactor = eVSF_HQCIF;
    if(config_init() < 0) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Configuration manager not initialized successfully\n",__FILE__, __LINE__);
	return;
    }
    memset(license,0,sizeof(license));
    strncpy(license,"jp394jzMOE54nf7oa7uZLEN587b5",LICSIZE);
    license[LICSIZE] = '\0';
//    interface = new pluginInterface();
//    temp_factory = CreatePluginFactoryInstance(); //creating plugin factory instance
//    recorder = (RdkCVideoCapturer*)temp_factory->CreateVideoCapturer();
}

/** @descripion: Destructor for Intellivision Engine
 *
 */
xcvAnalyticsEngine_Intellivision::~xcvAnalyticsEngine_Intellivision()
{

}

/** @descripion: Initializer for Intellivision Engine
 *  @return void
 */
int xcvAnalyticsEngine_Intellivision::Init()
{
    //Engine will stop working after running for 24 hours if no license.
    ivEngineManager_SetLicense(license);
    ivEngineManager_GetLicenseInfo(&info);
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):licenseStatus:%d, apiType:%d, versionString:%s, buildDate:%s\n",
    __FILE__,__LINE__,info.licenseStatus, info.apiType, info.versionString, info.buildDate);
    ivengine = ivEngineManager_Init();
    if (NULL == ivengine) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):ivEngineManager_Init: FAIL \n",__FILE__,__LINE__);
	return XCV_FAILURE;
    }

    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):SetVideoFormat: size: %dx%d\n", __FILE__,__LINE__,width, height);
    ivEngineManager_SetVideoFormat(ivengine, format, width, height, scaleFactor);

    if( RDKC_SUCCESS == ReadSensitivityParam() ) {
        RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VIDEOANALYTICS","%s(%d): day mode sensitivity: %.2f, night mode sensitivity: %.2f\n",__FUNCTION__, __LINE__,day_sen,night_sen);
    }
    else {
         RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):ReadSensitivityParam fail\n",__FILE__,__LINE__);
         printf("%s(%d):ReadSensitivityParam fail\n",__FILE__,__LINE__);
	 return XCV_FAILURE;
    }

    GetProperty();
    float val = 5.0;
    int ret = ivEngineManager_SetProperty(ivengine, I_PROP_PROPERTY_SET, (const void*)&val, 1);

    if(IV_OK != ret) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_PROPERTY failed\n", __FILE__,__LINE__);
	return XCV_FAILURE;
    }
    else {
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tSet I_PROP_PROPERTY TO 5.0 OK.\n",__FILE__,__LINE__);
    }
    return XCV_SUCCESS;
}

/** @descripion: Initializiation to be done only for first time for Intellivision Engine
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::InitOnce()
{
    GetProperty();
    SetProperty();
    GetProperty();
    return;
}

/** @descripion: Function to get events count
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::GetEventsCount()
{
    ivEngineManager_GetEventsCount(ivengine, &eventsCount);
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):%d events detected!\n", __FILE__,__LINE__,eventsCount);
    return;
}

/** @descripion: Function to get events
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::GetEvents()
{
    ivEngineManager_GetEvents(ivengine, events, MAX_EVENTS_NUM, &eventsCount);
    return;
}

/** @descripion: Function to get object count
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::GetObjectsCount()
{
    ivEngineManager_GetObjectsCount(ivengine, &objectsCount);
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):  objectsCount : %d \n ", __FILE__,__LINE__,objectsCount);
    return;
}

/** @descripion: Function to get objects
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::GetObjects()
{
    ivEngineManager_GetObjects(ivengine, objects, OD_MAX_NUM, &objectsCount);
    return;
}

int xcvAnalyticsEngine_Intellivision::GetMotionLevel(float *value)
{
    int elemCount = 1;
    float val = 0.0;
    int ret = ivEngineManager_GetProperty(ivengine, I_PROP_MOTION_LEVEL, (void*) &val, &elemCount);

    if( IV_OK == ret ) {
	*value = val;
	RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Get Motion Level: %.2f \n ", __FILE__,__LINE__,val);
    }
    else {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Get I_PROP_MOTION_LEVEL failed: %d\n", __FILE__,__LINE__,ret);
    }
    return 0;
}

/** @descripion: Function to get specific property from engine
 *  @paremeters:
 *       property [in] property to get value
 *       val [out] val property value
 *  @return int
 */
int xcvAnalyticsEngine_Intellivision::GetSpecificProperty(int property, float &val)
{
    return 0;
}

/** @descripion: Function to get motion data
 *  @paremeters:
 *        val [out] motion property value
 *  @return int
 */
void xcvAnalyticsEngine_Intellivision::GetRawMotion(unsigned long long framePTS,float &value)
{
    int elemCount = 1;
    float val =0.0;
    int ret = ivEngineManager_GetProperty(ivengine, I_PROP_MOTION_LEVEL_RAW, (void*) &val, &elemCount);
    if(IV_OK == ret) {
        value = val;
        RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Get Motion Level Raw: %.2f \n", __FILE__,__LINE__,val);
    }
    else {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Get I_PROP_MOTION_LEVEL_RAW failed: %d\n", __FILE__,__LINE__,ret);
    }
    //(iavInterfaceAPI::get_iva_structure())->timestamp = framePTS;
    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","I_PROP_MOTION_LEVEL_RAW: %d \n",val);
    return;
}

/** @descripion: Function to get engine properties
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::GetProperty()
{
    float val = 0;
    int elemCount = 1;
    iObjectClassData obj;
    int ret = 0;

    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV", "%s(%d):Get property: \n",__FILE__,__LINE__);

    ret = ivEngineManager_GetProperty(ivengine, I_PROP_CAMERA_CHECK_ENABLED, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_CAMERA_CHECK_ENABLED failed\n", __FILE__,__LINE__);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_CAMERA_CHECK_ENABLED: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_MOTION_ENABLED, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_MOTION_ENABLED failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_MOTION_ENABLED: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_CLASS_DETECTORS_PER_SAMPLE, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_CLASS_DETECTORS_PER_SAMPLE failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_OBJ_CLASS_DETECTORS_PER_SAMPLE: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_CLASS_SAMPLING_PERIOD, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_CLASS_SAMPLING_PERIOD failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_OBJ_CLASS_SAMPLING_PERIOD: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_CLASS_ACCURACY_PRESET, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_CLASS_ACCURACY_PRESET failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_OBJ_CLASS_ACCURACY_PRESET: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_CLASS_CPU_QUOTA, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_CLASS_CPU_QUOTA failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_OBJ_CLASS_CPU_QUOTA: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_CAMERA_VIEW, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_CAMERA_VIEW failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_CAMERA_VIEW: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_SCENE_TYPE, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_SCENE_TYPE failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_SCENE_TYPE: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_SENSITIVITY, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_SENSITIVITY failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_SENSITIVITY: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_OTHER_WIDTH_MIN, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_OTHER_WIDTH_MIN failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tGet I_PROP_OBJ_OTHER_WIDTH_MIN:%.2f.\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_OTHER_WIDTH_MAX, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_OTHER_WIDTH_MAX failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tGet I_PROP_OBJ_OTHER_WIDTH_MAX:%.2f.\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_OTHER_HEIGHT_MIN, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_OTHER_HEIGHT_MIN failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tGet I_PROP_OBJ_OTHER_HEIGHT_MIN:%.2f.\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_OTHER_HEIGHT_MAX, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_OTHER_HEIGHT_MAX failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tGet I_PROP_OBJ_OTHER_HEIGHT_MAX:%.2f.\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_CLASS_ENABLED, (void*) &val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_CLASS_ENABLED failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_OBJ_CLASS_ENABLED: %.2f\n", __FILE__,__LINE__,val);
    }
   
    elemCount = 1;
    memset(&obj, 0, sizeof(obj));
    obj.m_e_class = eOC_Human;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_CLASS_DATA, (void*) &obj, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_CLASS_DATA(Human) failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\teOC_Human.m_i_classEnabled: %d\n", __FILE__,__LINE__,obj.m_i_classEnabled);
    }

    elemCount = 1;
    memset(&obj, 0, sizeof(obj));
    obj.m_e_class = eOC_Vehicle;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_OBJ_CLASS_DATA, (void*) &obj, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_OBJ_CLASS_DATA(Vehicle) failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\teOC_Vehicle.m_i_classEnabled: %d\n", __FILE__,__LINE__,obj.m_i_classEnabled);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_CAMERA_TYPE, (void*)&val, &elemCount);
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\teFixedCameraType=0,ePTZCameraType=1,eMovingCameraType=2,e360DegreeCameraType=3.\n",__FILE__,__LINE__);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_CAMERA_TYPE failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_CAMERA_TYPE: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;// Make sure this get ok,this may similar to eOC_Vehicle
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_VIDEO_SCALE_FACTOR, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_VIDEO_SCALE_FACTOR failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_VIDEO_SCALE_FACTOR: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_PROCESSING_FPS, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_PROCESSING_FPS: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_PROCESSING_FPS: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_ACTIVITY_ENABLED, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_ACTIVITY_ENABLED: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_ACTIVITY_ENABLED: %.2f\n", __FILE__,__LINE__,val);
    }

    //get magic property
    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, 2103, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get MAGIC Property 2103 failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tMAGIC PROPERTY: %0.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_LIGHT_CHANGE_DETECTOR_ENABLED, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_LIGHT_CHANGE_DETECTOR_ENABLED: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_LIGHT_CHANGE_DETECTOR_ENABLED: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_LIGHT_CHANGE_SENSITIVITY, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_LIGHT_CHANGE_SENSITIVITY: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_LIGHT_CHANGE_SENSITIVITY: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_BG_INITIAL_LEARN_TIME, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_BG_INITIAL_LEARN_TIME: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_BG_INITIAL_LEARN_TIME: %.2f\n", __FILE__,__LINE__,val);
    }

    elemCount = 1;
    ret = ivEngineManager_GetProperty(ivengine, I_PROP_FG_EXTRACTOR_MODE, (void*)&val, &elemCount);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: get I_PROP_FG_EXTRACTOR_MODE: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tI_PROP_FG_EXTRACTOR_MODE: %.2f\n", __FILE__,__LINE__,val);
    }
    return;
}

/** @description: set upscale resolution
 *  @param: day_night_mode : int
 */
int xcvAnalyticsEngine_Intellivision::SetUpscaleResolution(eRdkCUpScaleResolution_t resolution)
{
    /* TODO: Add the logic to configure the upscaling resolution here */
    return RDKC_SUCCESS;
}


/** @descripion: Function to set engine properties
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::SetProperty()
{
    float val = 1.0;
    int ret = 0;

    FILE *fd = NULL;
    int scene = SCENE1;
    int sen = DEFAULT_IV_DAY_MODE_SENSITIVITY;
    int hd_mode = -1;
    int td_mode = -1;
    const char* env = NULL;

    fd = fopen(SYSTEM_CONF,"rt");
    PRO_GetInt(SEC_MOTION, SCENE, &scene, fd);
//  PRO_GetInt(SEC_MOTION, MOTION_SENSITIVITY1, &sen, fd);
    PRO_GetInt(SEC_MOTION, HUMAN_DETECT, &hd_mode, fd);
    PRO_GetInt(SEC_MOTION, SCENE, &td_mode, fd);
//  PRO_GetInt(SEC_MOTION, FACE_DETECT, &fd_mode, fd);
    fclose(fd);

    if( DAY_MODE == curr_day_night_mode ) {
        sen = day_sen;
    }
    else if( NIGHT_MODE == curr_day_night_mode ) {
	sen = night_sen;
    }
    else {
            RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","Could not read Day-Night mode. Setting sensitivity to default\n");
            sen = DEFAULT_IV_DAY_MODE_SENSITIVITY;
    }
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):set property: \n",__FILE__,__LINE__);

    val = 1.0;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_ACTIVITY_ENABLED, (const void*)&val, 1);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_ACTIVITY_ENABLED failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tSet I_PROP_ACTIVITY_ENABLED TO 1.0 OK.\n",__FILE__,__LINE__);
    }

    //set magic property
    val = 1.0;
    ret = ivEngineManager_SetProperty(ivengine, 2103, (const void*)&val, 1);
    if(IV_OK != ret) {
       RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: Set MAGIC PROPERTY 2103 failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
       RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tSet MAGIC PROPERTY 2103 TO 1.0 OK.\n",__FILE__,__LINE__);
    }

    val = 1.0;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_CAMERA_CHECK_ENABLED, (const void*)&val, 1);
    if(IV_OK != ret) {
       RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_CAMERA_CHECK_ENABLED failed: %d\n",__FILE__,__LINE__,ret);
    }
    else {
       RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tSet I_PROP_CAMERA_CHECK_ENABLED OK.\n",__FILE__,__LINE__);
    }

    val = 1.0;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_MOTION_ENABLED, (const void*)&val, 1);
    if(IV_OK != ret) {
       RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_MOTION_ENABLED failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
       RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tSet I_PROP_MOTION_ENABLED OK.\n",__FILE__,__LINE__);
    }

    val = 0;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_MOTION_SMART_MOTION, (const void*)&val, 1);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_MOTION_SMART_MOTION failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tSet I_PROP_MOTION_SMART_MOTION OK.\n",__FILE__,__LINE__);
    }

    if (SCENE1 == scene) {
        val = 1.0;
    }
    else if (SCENE2 == scene) {
        val = 0;
    }

    ret = ivEngineManager_SetProperty(ivengine, I_PROP_SCENE_TYPE, (const void*)&val, 1);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_SCENE_TYPE failed: %d\n",__FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tSet I_PROP_SCENE_TYPE OK.\n",__FILE__,__LINE__);
    }

    val = (float)sen * 10;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_SENSITIVITY, (const void*)&val, 1);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_SENSITIVITY failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tSet I_PROP_SENSITIVITY OK.\n",__FILE__,__LINE__);
    }

    //set human detection chassifier
    iObjectClassData human;
    memset(&human, 0, sizeof(human));
    human.m_e_class = eOC_Human;
    human.m_i_classEnabled = hd_mode;
    human.m_i_advancedValidation = 1;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_OBJ_CLASS_DATA, &human, 1);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_OBJ_CLASS_DATA(Human) failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tEnable I_PROP_OBJ_CLASS_DATA(Human) OK.\n",__FILE__,__LINE__);
    }

#ifdef IVA_ENABLE_VEHICLE_CLASSIFIER
    iObjectClassData car;
    memset(&car, 0, sizeof(car));
    car.m_e_class = eOC_Vehicle;
    car.m_i_classEnabled = 1;
    car.m_i_advancedValidation = 1;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_OBJ_CLASS_DATA, &car, 1);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: set I_PROP_OBJ_CLASS_DATA(Vehicle) failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tEnable I_PROP_OBJ_CLASS_DATA(Vehicle) OK.\n",__FILE__,__LINE__);
    }
#endif

    //set I_PROP_PROCESSING_FPS
    val = 8.0;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_PROCESSING_FPS, (const void*) &val, 1);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: I_PROP_PROCESSING_FPS failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tEnable I_PROP_PROCESSING_FPS OK.\n",__FILE__,__LINE__);
    }

    //set I_PROP_FG_EXTRACTOR_MODE
    val = eFGMode_Advanced;
    ret = ivEngineManager_SetProperty(ivengine, I_PROP_FG_EXTRACTOR_MODE, (const void*) &val, 1);
    if(IV_OK != ret) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError: I_PROP_FG_EXTRACTOR_MODE failed: %d\n", __FILE__,__LINE__,ret);
    }
    else {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tEnable I_PROP_FG_EXTRACTOR_MODE OK.\n",__FILE__,__LINE__);
    }

    return;
}

/** @descripion: Function to process frame
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::ProcessFrame()
{
    using namespace std::chrono_literals;
    //xcvInterface::get_iDateTime(&dateTime);

    // Initialize timeStamp with current frame timestamp in ms
    //timeStamp = iavInterfaceAPI::getmsec();

    // process frame
    // plane0 is Y value, plane1 is interleaved UV
    RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV","%s(%d): do detection processing...\n" ,__FILE__, __LINE__);

    ivEngineManager_Process(ivengine, &plane0, &plane1, 0, &dateTime, timeStamp);
    return;
}

/** @descripion: Function to shut down Intellivision analytics engine
 *  @return void
 */
void xcvAnalyticsEngine_Intellivision::Shutdown()
{
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):Engine Shutdown ",__FUNCTION__, __LINE__);
    if(NULL != ivengine) {
        ivEngineManager_Release(&ivengine);
    }
    return;
}

/** @description: Read day/night sensitivity value from config file
 *  @param:void
 *  @return: sensitivity value
 */
int xcvAnalyticsEngine_Intellivision::ReadSensitivityParam()
{
    char* configParam = NULL;
    char config_param[CONFIG_PARAM_MAX_LENGTH];
    memset(config_param, 0, CONFIG_PARAM_MAX_LENGTH);

    /* Read Day mode sensitivity for IV from configuration file */
    if (RDKC_SUCCESS != rdkc_get_user_setting(IV_DAY_SENSITIVITY, config_param)) {
        configParam = (char*)rdkc_envGet(IV_DAY_SENSITIVITY);
    }
    else {
        configParam = config_param;
    }

    if( NULL == configParam ) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: Could not read Day mode sensitivity for IV from config file. Setting it to default %d\n",DEFAULT_IV_DAY_MODE_SENSITIVITY);
        day_sen = DEFAULT_IV_DAY_MODE_SENSITIVITY;
    }
    else {
        day_sen = atof(configParam);

        if( day_sen <= IV_MIN_SENSITIVITY || day_sen > IV_MAX_SENSITIVITY ) {
            RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: Invalid Day mode sensitivity for IV.Setting it to default %d\n",DEFAULT_IV_DAY_MODE_SENSITIVITY);
            day_sen = DEFAULT_IV_DAY_MODE_SENSITIVITY;
        }
    }

    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","IV Day mode sensitivity is set to %f.\n",day_sen);
    /* Read Night mode sensitivity for IV from configuration file */
    if (RDKC_SUCCESS != rdkc_get_user_setting(IV_NIGHT_SENSITIVITY, config_param)) {
        configParam = (char*)rdkc_envGet(IV_NIGHT_SENSITIVITY);
    }
    else {
        configParam = config_param;
    }

    if( NULL == configParam ) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: Could not read Night mode sensitivity for IV from config file. Setting it to default %d\n", DEFAULT_IV_NIGHT_MODE_SENSITIVITY);
        night_sen = DEFAULT_IV_NIGHT_MODE_SENSITIVITY;
    }
    else {
        night_sen = atof(configParam);

       if( night_sen <= IV_MIN_SENSITIVITY || night_sen > IV_MAX_SENSITIVITY ) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: Invalid Night mode sensitivity for IV.Setting it to default %d\n",DEFAULT_IV_NIGHT_MODE_SENSITIVITY);
        night_sen = DEFAULT_IV_NIGHT_MODE_SENSITIVITY;
       }
    }

    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","IV Night mode sensitivity is set to %f.\n",night_sen);
    return RDKC_SUCCESS;
}

/** @description: set IV sensitivity according to Day/Night mode
 *  @param[in] curr_day_night_mode : current camera mode (Day/Night)
 *  @return: RDKC_SUCCESS on success, RDKC_FAILURE on failure
 */
int xcvAnalyticsEngine_Intellivision::SetSensitivity(int curr_day_night_mode)
{
    float sen = 0.0;

    if( DAY_MODE == curr_day_night_mode ) {
        sen = day_sen;
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","IV Day mode sensitivity is set to %f.\n",sen);
    }
    else if( NIGHT_MODE == curr_day_night_mode ) {
        sen = night_sen;
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","IV Night mode sensitivity is set to %f.\n",sen);
    }
    /* Set IV Engine Sensitivity */
    float val = (float)sen * 10;
    if( IV_OK != ivEngineManager_SetProperty(ivengine, I_PROP_SENSITIVITY, (const void*)&val, 1) ) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","\tError: set I_PROP_SENSITIVITY failed\n");
        return RDKC_FAILURE;
    }
    else {
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","\tSet I_PROP_SENSITIVITY OK.\n");
    }
    return RDKC_SUCCESS;
}

/** @description: set Day night mode
 *  @param: day_night_mode int
 */
int xcvAnalyticsEngine_Intellivision::SetDayNightMode(int day_night_mode)
{
    curr_day_night_mode = day_night_mode;
    return XCV_SUCCESS;
}

/** @descripion: Function to reset Engine 
 *  @param: engine - xcvAnalyticsEngine pointer
 *  frametype - int 
 */
int xcvAnalyticsEngine_Intellivision::reset(xcvAnalyticsEngine* engine, int frametype)
{
    if(NULL == engine) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): xcvAnalyticsEngine is NULL",__FUNCTION__,__LINE__);
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
int xcvAnalyticsEngine_Intellivision::GetAlgIndex()
{
    return IV; 
}

/** @descripion: Function to get ScaleFactor
 *  @return int
 */
int xcvAnalyticsEngine_Intellivision::GetScaleFactor()
{
    return scaleFactor;
}

/** @descripion: Function to get Format
 *  @return int
 */
int xcvAnalyticsEngine_Intellivision::GetFormat()
{
    return format;
}
/** @description: Get VA engine version 
 *  @return int
 */
int xcvAnalyticsEngine_Intellivision::GetEngineVersion() { return 0;}

/** @description: Get motion score 
 *  @return int
 */
int xcvAnalyticsEngine_Intellivision::GetMotionScore() { return 0;}

/** @description: Get Object Box Coords 
 *  @return int 
 */
int xcvAnalyticsEngine_Intellivision::GetObjectBBoxCoords() { return 0;}

/** @descripion: Function to get Iv engine pointer
 *  @return void *
 */
void* xcvAnalyticsEngine_Intellivision::GetIVPointer()
{
    return ivengine;
}

#ifdef _ROI_ENABLED_

/** @descripion: set ROI
 *  @param: coords - coordinates list
 *  @return int
 */
int xcvAnalyticsEngine_Intellivision::SetROI(std::vector<float> coords)
{
    /* TODO : set ROI Coordinatres*/
    return XCV_SUCCESS;
}

/** @descripion: get ROI
 *  @param:
 *  @return coords - coordinates list
 */
std::vector<float> xcvAnalyticsEngine_Intellivision::GetROI()
{
    /* TODO : Get ROI Coordinatres */
    std::vector<float> coords;
    coords.clear();
    return coords;
}

/** @descripion: Clear ROI
 *  @param:
 *  @return int
 */
int xcvAnalyticsEngine_Intellivision::ClearROI()
{
    /* TODO : Clear ROI Coordinatres*/
    return XCV_SUCCESS;
}

/** @descripion: Check if motion is inside ROI
 *  @param:
 *  @return bool
 */
bool xcvAnalyticsEngine_Intellivision::IsMotionInsideROI()
{
    /* TODO: check if motion is inside ROI */
    return false;
}

/** @description: Check if ROI set
 *  @param:
 *  @return 1 if set, else 0
 */
int xcvAnalyticsEngine_Comcast::IsROISet()
{
    return 0;
}

#endif

bool xcvAnalyticsEngine_Intellivision::applyDOIthreshold(bool enable, char *doi_path, int doi_threshold) {
    /* TODO: apply DOI */
    return false;
}

/** @descripion: Check if motion is inside DOI
 *  @param:
 *  @return bool
 */
bool xcvAnalyticsEngine_Intellivision::IsMotionInsideDOI()
{
    /* TODO: check if motion is inside DOI */
    return false;
}

/** @description: Check if DOI set
 *  @param:
 *  @return bool
 *  @return 1 if set, else 0
 */
int xcvAnalyticsEngine_Comcast::IsDOISet()
{
    return 0;
}

#ifdef _OBJ_DETECTION_
int xcvAnalyticsEngine_Intellivision::GetDetectionObjectBBoxCoords()
{
    return XCV_SUCCESS;
}

int xcvAnalyticsEngine_Intellivision::SetDeliveryUpscaleFactor(float scaleFactor)
{
    return XCV_SUCCESS;
}
#endif

int xcvAnalyticsEngine_Intellivision::SetDOIOverlapThreshold(float threshold)
{
    return XCV_SUCCESS;
}

/** @descripion: Function to create Intellivision analytics engine
 *  @return xcvAnalyticsEngine_Intellivision object
 */
xcvAnalyticsEngine* CreateEngine()
{
    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):\tCreate Engine\n",__FILE__,__LINE__);
    return new xcvAnalyticsEngine_Intellivision();
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
