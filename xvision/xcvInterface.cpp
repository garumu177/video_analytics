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
#include <string>
#include <cmath>
#include <sstream>
#include "xcvInterface.h"
#define KDOIBITMAPDEFAULTTHRESHOLD 80

double xcvInterface::od_scale_table[OD_RAW_RES_MAX][IMG_RES_NUM];
int xcvInterface::od_crop_table[OD_RAW_RES_MAX][IMG_RES_NUM];
int xcvInterface::od_crop_max_width[OD_RAW_RES_MAX];

char xcvInterface::doibitmapPath[MAX_CONFIG_LENGTH];
bool xcvInterface::doienabled = false;
int xcvInterface::doiBitmapThreshold = KDOIBITMAPDEFAULTTHRESHOLD;
#ifdef _ROI_ENABLED_
char xcvInterface::roiCoords[MAX_CONFIG_LENGTH];
#endif

#ifdef RTMSG
rtError xcvInterface::err;
rtMessage xcvInterface::m;
rtConnection xcvInterface::connectionSend;
rtConnection xcvInterface::connectionRecv;
bool xcvInterface::rtmessageXCVThreadExit = false;
volatile bool xcvInterface::smartTnEnabled = false;
volatile bool xcvInterface::hasROIChanged = false;
volatile bool xcvInterface::hasDOIChanged = false;
pthread_t xcvInterface::rtMessageRecvThread;
#endif

vai_result_t* xcvInterface::vai_result = NULL;
#ifdef ENABLE_TEST_HARNESS
THInterface * xcvInterface::th_interface = NULL;
#endif


#define str_objCategory(n,str,len) do {\
 switch(n)\
  {\
   case eLeft: strncpy(str, "eLeft",len); break;\
   case eUncategorized: strncpy(str, "eUncategorized",len);break;\
   case eRemoved: strncpy(str, "eRemoved",len); break;\
   case eSuspected: strncpy(str, "eSuspected",len); break;\
  }\
 }while(0);

#define str_objClass(n,str,len) do {\
 switch(n)\
  {\
   case eOC_Human: strncpy(str, "eOC_Human",len); break;\
   case eOC_Unknown: strncpy(str, "eOC_Unknown",len); break;\
   case eOC_Vehicle: strncpy(str, "eOC_Vehicle",len); break;\
   case eOC_Train: strncpy(str, "eOC_Train",len); break;\
   case eOC_Pet: strncpy(str, "eOC_Pet",len); break;\
   case eOC_Other: strncpy(str, "eOC_Other",len); break;\
   case eOC_Any: strncpy(str, "eOC_Any",len); break;\
  }\
 }while(0);

 #define str_eventType(n,str,len) do {\
  switch(n)\
   {\
    case eIntrusionEvent: strncpy(str, "Intrusion", len); break;\
    case eObjectEnteredEvent: strncpy(str, "Object Entered", len); break;\
    case eObjectExitedEvent: strncpy(str, "Object Exited", len); break;\
    case eIntrusionEnteredEvent: strncpy(str, "Intrusion Entered", len); break;\
    case eIntrusionExitedEvent: strncpy(str, "Intrusion Exited", len); break;\
    case eObjectLost: strncpy(str, "Object Lost", len); break;\
    case eObjectCrossedIn: strncpy(str, "Object Crossed In", len); break;\
    case eObjectCrossedOut: strncpy(str, "Object Crossed Out", len); break;\
    case eLightsOff: strncpy(str, "Lights Off", len); break;\
    case eLightsOn: strncpy(str, "Lights On", len); break;\
    case eSceneChange: strncpy(str, "Camera Tampered", len); break;\
    case eObjectLeftEvent: strncpy(str, "Object Left", len); break;\
    case eObjectRemovedEvent: strncpy(str, "Object Removed", len); break;\
    case ePersonFallEvent: strncpy(str, "Person Fall", len); break;\
    case ePersonDuressEvent: strncpy(str, "Person Duress", len); break;\
    case eDwellEnterEvent: strncpy(str, "Object Entered AdMonitor Area", len); break;\
    case eDwellExitEvent: strncpy(str, "Object Exited AdMonitor Area", len); break;\
    case eDwellLoiteringEvent: strncpy(str, "Object Loitering", len); break;\
    case eWrongDirection: strncpy(str, "Wrong Direction", len); break;\
    case eMotionDetected: strncpy(str, "Motion Detected", len); break;\
    case eLineCrossed: strncpy(str, "Line Crossed", len); break;\
    case eDefocused: strncpy(str, "De Focus", len); break; \
    case ePersonAbnormalEvent: strncpy(str, "Abnormal Event", len); break;\
    case eObjectSpeedLow: strncpy(str, "Object Speed Low", len); break;\
    case eObjectStopped: strncpy(str, "Object Stopped", len); break;\
    case ePersonRunningEvent: strncpy(str, "Person Running", len); break;\
    case eCrowdDetectedEvent: strncpy(str, "Crowd Detected", len); break;\
    case eQueueEnteredEvent: strncpy(str, "Object Evented Queue", len); break;\
    case eQueueExitedEvent: strncpy(str, "Object Exited Queue", len); break;\
    case eQueueServicedEvent: strncpy(str, "Object Serviced in Queue", len); break;\
    case eLossOfVideo: strncpy(str, "Loss of Video", len); break;\
    case eUnknownEvent: strncpy(str, "eUnknownEvent", len); break;\
   }\
 }while(0);

/** @descripion: This function is used to print the objects along with category
 *  @parameter:
 *       object
 *  @return:
 *       int 0 for success
 */
int xcvInterface::print_object(iObject *object)
 {
   char description[64]= "";
   str_objCategory(object->m_e_objCategory, description, sizeof(description));

   RDK_LOG(RDK_LOG_DEBUG1," %s(%d) objCategory:%s \n ", __FILE__, __LINE__, description);
   str_objClass(object->m_e_class, description, sizeof(description));

   RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV"," %s(%d) objClass:%s \n",__FILE__, __LINE__,description);

   RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV"," %s(%d) m_i_ID:%d, state:%d, confidence:%f, Box.lCol:%d, Box.tRow:%d, Box.rCol:%d, Box.bRow:%d\n",
      __FILE__, __LINE__, object->m_i_ID, object->m_e_state, object->m_f_confidence,
      object->m_u_bBox.m_i_lCol, object->m_u_bBox.m_i_tRow, object->m_u_bBox.m_i_rCol, object->m_u_bBox.m_i_bRow);
   return XCV_SUCCESS;
 }

/** @descripion: This function is used to print the events along with type
 *  @parameter:
 *       event
 *  @return:
 *       int 0 for success
 */
int xcvInterface::print_event(iEvent *event)
 {
   char description[64] = "";
   str_eventType(event->m_e_type, description, sizeof(description));
   RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV"," %s(%d) eventType:%s, time:%d.%d.%d %d:%d:%d\n", __FILE__, __LINE__, description,
      event->m_dt_time.m_i_year, event->m_dt_time.m_i_month, event->m_dt_time.m_i_day, event->m_dt_time.m_i_hour,
      event->m_dt_time.m_i_minute, event->m_dt_time.m_i_second);
   return XCV_SUCCESS;
 }

/** @descripion: This function is used to print date and time
 *  @parameter:
 *       dateTime
 *  @return:
 *       int 0 for success
 */
int xcvInterface::get_iDateTime(iDateTime *dateTime)
 {
   time_t time_now;
   struct tm tm_now;
   time(&time_now);
   localtime_r(&time_now, &tm_now);
   dateTime->m_i_year = tm_now.tm_year + 1900;
   dateTime->m_i_month = tm_now.tm_mon + 1;
   dateTime->m_i_day = tm_now.tm_mday;
   dateTime->m_i_hour = tm_now.tm_hour;
   dateTime->m_i_minute = tm_now.tm_min;
   dateTime->m_i_second = tm_now.tm_sec;
   RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): date:%d-%d-%d %d:%d:%d\n",__FILE__, __LINE__,dateTime->m_i_year, dateTime->m_i_month, dateTime->m_i_day, dateTime->m_i_hour,dateTime->m_i_minute, dateTime->m_i_second);
   return XCV_SUCCESS;
 }

 /** @descripion: This function is used to dump the yuv raw data that will be played by YUV palyer
 *  @parameter:
 *       y_addr
 *       width
 *       height
 *  @return:
 *       int 0 for success
 */
int xcvInterface::dump_raw_data(unsigned char *y_addr, unsigned char *uv_addr, int width, int height)
 {
   int written = 0;
   int size = width * height;
   int uv_size = size/2;           //U and V are 1/4 of Y
   unsigned char *uv_data, *u_addr, *v_addr;
   int fd = -1;
   int i = 0;
   fd= creat("/tmp/iav_raw_dump.yuv", 0666);
   if (fd < 0) {
     RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV"," %s(%d) fd error!\n", __FILE__,__LINE__);
     return XCV_FAILURE;
    }
   //fill the UV data for the YUVpalyer to play
   uv_data = (unsigned char* )malloc(uv_size);
   if(NULL == uv_data) {
     RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV"," %s(%d) malloc error!\n",__FILE__, __LINE__);
     close(fd);
     return XCV_FAILURE;
    }

   u_addr = uv_data;
   v_addr = uv_data + uv_size/2;
   i = uv_size/2;
   while(i-- > 0) {
     *u_addr++ = *uv_addr++;
     *v_addr++ = *uv_addr++;
    }

   //write Y data
   written = 0;
   while (written < size) {
     written += write(fd, y_addr+written, size-written);
     if(written < 0) {
       RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV"," %s(%d) write error: errno=%d\n", __FILE__, __LINE__,errno);
       free(uv_data);
       close(fd);
       return XCV_FAILURE;
      }
    }
   RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV"," %s(%d) write Y data %d\n",__FILE__, __LINE__,written);
   //write UV data
   written = 0;
   while (written < uv_size) {
     written += write(fd, uv_data+written, uv_size-written);
     if(written < 0) {
       RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV"," %s(%d) write error: errno=%d\n", __FILE__, __LINE__,errno);
       free(uv_data);
       close(fd);
       return XCV_FAILURE;
      }
     RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV"," %s(%d) write UV data %d\n", __FILE__, __LINE__,written);
    }

    free(uv_data);
    close(fd);
    RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV"," %s(%d) raw data dumped\n", __FILE__, __LINE__);
    return XCV_SUCCESS;
 }

#ifdef RTMSG

/** @description:Dispatch the message received
 *  @param[in] args : void pointer
 *  @return: void pointer
 */
void* xcvInterface::receive_rtmessage(void *args)
{
        while (!rtmessageXCVThreadExit) {
                rtError err = rtConnection_Dispatch(connectionRecv);
                if (RT_OK != err) {
                        RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.DYNAMICLOG","%s(%d): Dispatch Error: %s", __FILE__, __LINE__, rtStrError(err));
                }
        }
        return NULL;
}

/** @description:Callback function for the message received
 *  @param[in] hdr : pointer to rtMessage Header
 *  @param[in] buff : buffer for data received via rt message
 *  @param[in] n : number of bytes received
 *  @return: void
 */
void xcvInterface::dynLogOnMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
        char const*  module = NULL;
        char const*  logLevel = NULL;

        rtConnection con = (rtConnection) closure;

        rtMessage req;
        rtMessage_FromBytes(&req, buff, n);

        //Handle the rtmessage request
        if (rtMessageHeader_IsRequest(hdr))
        {
                char* tempbuff = NULL;
                uint32_t buff_length = 0;

                rtMessage_ToString(req, &tempbuff, &buff_length);
                rtLog_Info("Req : %.*s", buff_length, tempbuff);
                free(tempbuff);

                rtMessage_GetString(req, "module", &module);
                rtMessage_GetString(req, "logLevel", &logLevel);

                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.DYNAMICLOG","(%s):%d Module name: %s\n", __FUNCTION__, __LINE__, module);
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.DYNAMICLOG","(%s):%d log level: %s\n", __FUNCTION__, __LINE__, logLevel);

                RDK_LOG_ControlCB(module, NULL, logLevel, 1);

                // create response
                rtMessage res;
                rtMessage_Create(&res);
                rtMessage_SetString(res, "reply", "Success");
                rtConnection_SendResponse(con, hdr, res, 1000);
                rtMessage_Release(res);
        }
        rtMessage_Release(req);
	return;
}

#ifdef ENABLE_TEST_HARNESS
void xcvInterface::set_thinterface(THInterface *th)
{
	th_interface = th;
	return;
}
void xcvInterface::smtTnOnUpload(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
    th_smtnUploadStatus stnStatus;
    rtMessage m;
    rtMessage_FromBytes(&m, buff, n);

    char* tempbuff = NULL;
    uint32_t buff_length = 0;
    rtMessage_ToString(m, &tempbuff, &buff_length);
    rtLog_Debug("Req : %.*s", buff_length, tempbuff);
    free(tempbuff);
    rtMessage_GetInt32(m, "status", &(stnStatus.status));
    RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV","(%s):%d smart thumbnail upload payload received status: %s\n", __FUNCTION__, __LINE__, stnStatus.status);
    if(th_interface) th_interface->THAddUploadStatus(stnStatus);
    else RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","(%s):%d th_interface NULL\n", __FUNCTION__, __LINE__);
    rtMessage_Release(m);
}
void xcvInterface::smtTnOnDeliveryData(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
	std::string timestamp;
	int len;
	th_deliveryResult result;

        rtMessage req;
        rtMessage_FromBytes(&req, buff, n);

        char* tempbuff = NULL;
        uint32_t buff_length = 0;

        rtMessage_ToString(req, &tempbuff, &buff_length);
        rtLog_Debug("Req : %.*s", buff_length, tempbuff);
        free(tempbuff);

	rtMessage_GetInt32(req, "FileNum", &(result.fileNum));
	rtMessage_GetInt32(req, "FrameNum", &(result.frameNum));
	rtMessage_GetDouble(req, "deliveryConfidence", &(result.deliveryScore));
        rtMessage_GetInt32(req, "maxAugScore", &(result.maxAugScore));
        rtMessage_GetDouble(req, "motionTriggeredTime", &(result.motionTriggeredTime));
        rtMessage_GetInt32(req, "mpipeProcessedframes", &(result.mpipeProcessedframes));
        rtMessage_GetDouble(req, "time_taken", &(result.time_taken));
        rtMessage_GetDouble(req, "time_waited", &(result.time_waited));

	rtMessage_GetArrayLength(req, "Persons", &len);

	for (int32_t i = 0; i < len; i++) {
		int x, y, w, h;
		double c;
		std::vector<int> bbox;
		rtMessage personInfo;
		rtMessage_GetMessageItem(req, "Persons", i, &personInfo);
		rtMessage_GetInt32(personInfo, "boundingBoxXOrd", &(x));
		rtMessage_GetInt32(personInfo, "boundingBoxYOrd", &(y));
		rtMessage_GetInt32(personInfo, "boundingBoxWidth", &(w));
		rtMessage_GetInt32(personInfo, "boundingBoxHeight", &(h));
		bbox.push_back(x);
		bbox.push_back(y);
		bbox.push_back(w);
		bbox.push_back(h);
		result.personBBoxes.push_back(bbox);
		rtMessage_GetDouble(personInfo, "confidence", &(c));
		result.personScores.push_back(c);
		rtMessage_Release(personInfo);
	}

	rtMessage_GetArrayLength(req, "nonROIPersons", &len);

	for (int32_t i = 0; i < len; i++) {
		int x, y, w, h;
		double c;
		std::vector<int> bbox;
		rtMessage personInfo;
		rtMessage_GetMessageItem(req, "nonROIPersons", i, &personInfo);
		rtMessage_GetInt32(personInfo, "boundingBoxXOrd", &(x));
		rtMessage_GetInt32(personInfo, "boundingBoxYOrd", &(y));
		rtMessage_GetInt32(personInfo, "boundingBoxWidth", &(w));
		rtMessage_GetInt32(personInfo, "boundingBoxHeight", &(h));
		bbox.push_back(x);
		bbox.push_back(y);
		bbox.push_back(w);
		bbox.push_back(h);
		result.nonROIPersonBBoxes.push_back(bbox);
		rtMessage_GetDouble(personInfo, "confidence", &(c));
		result.nonROIPersonScores.push_back(c);
		rtMessage_Release(personInfo);
	}

        if(th_interface) th_interface->THAddDeliveryResult(result);
        else RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","(%s):%d th_interface NULL\n", __FUNCTION__, __LINE__);

	rtMessage_Release(req);
	return;
}
#endif

/** @description:Callback function for the message received
 *  @param[in] hdr : pointer to rtMessage Header
 *  @param[in] buff : buffer for data received via rt message
 *  @param[in] n : number of bytes received
 *  @return: void
 */

void xcvInterface::smtTnOnMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
        char const*  status = NULL;

        rtConnection con = (rtConnection) closure;

        rtMessage req;
        rtMessage_FromBytes(&req, buff, n);

        char* tempbuff = NULL;
        uint32_t buff_length = 0;

        rtMessage_ToString(req, &tempbuff, &buff_length);
        rtLog_Debug("Req : %.*s", buff_length, tempbuff);
        free(tempbuff);

        rtMessage_GetString(req, "status", &status);

        RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV","(%s):%d smart thumbnail received status: %s\n", __FUNCTION__, __LINE__, status);

        if(!strcmp(status, "start")) {
                smartTnEnabled = true;
                RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV","(%s):%d smart thumbnail is enabled.\n", __FUNCTION__, __LINE__);
                hasROIChanged = true;
        } else {
                smartTnEnabled = false;
                RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV","(%s):%d smart thumbnail is disabled.\n", __FUNCTION__, __LINE__);
        }
        rtMessage_Release(req);
	return;
}

/** @description:Callback function for the message received
 *  @param[in] void
 *  @return: bool
 */

bool xcvInterface::get_smart_TN_status() {

        return smartTnEnabled;
}

/** @description:Check ROI change status
 *  @param[in] void
 *  @return: bool
 */

bool xcvInterface::get_ROI_status() {

        return hasROIChanged;
}

/** @description:set_ROI_status(
 *  @param[in] void
 *  @return: bool
 */

void xcvInterface::set_ROI_status(bool status) {

        hasROIChanged = status;
	return;
}

/** @description:set_DOI_status
 *  @param[in] void
 *  @return: bool
 */

void xcvInterface::set_DOI_status(bool status) {

        hasDOIChanged = status;
	return;
}

/** @description:Check DOI change status
 *  @param[in] void
 *  @return: bool
 */

bool xcvInterface::get_DOI_status() {

        return hasDOIChanged;
}

#ifdef _ROI_ENABLED_
/** @description    : Callback function to update ROI configuration
 *  @param[in]  hdr : constant pointer rtMessageHeader
 *  @param[in] buff : constant pointer uint8_t
 *  @param[in]    n : uint32_t
 *  @param[in] closure : void pointer
 *  @return: void
 */
void xcvInterface::onMsgROIConfRefresh(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
    rtConnection con = (rtConnection) closure;

    rtMessage req;
    rtMessage_FromBytes(&req, buff, n);

    char* tempbuff = NULL;
    uint32_t buff_length = 0;
    const char * coords;

    rtMessage_ToString(req, &tempbuff, &buff_length);
    rtLog_Debug("Req : %.*s", buff_length, tempbuff);
    free(tempbuff);

    rtMessage_GetString(req, "coords", &coords);
    strcpy(roiCoords, coords);

    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","(%s):%d ROI Coords=%s\n", __FUNCTION__, __LINE__, roiCoords);

    hasROIChanged = true;

    rtMessage_Release(req);
    return;
}
#endif

/** @description    : Callback function to update DOI configuration
 *  @param[in]  hdr : constant pointer rtMessageHeader
 *  @param[in] buff : constant pointer uint8_t
 *  @param[in]    n : uint32_t
 *  @param[in] closure : void pointer
 *  @return: void
 */
void xcvInterface::onMsgDOIConfRefresh(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
    rtConnection con = (rtConnection) closure;
    const char * enabled = "false";
    const char *path = NULL;

    rtMessage req;
    rtMessage_FromBytes(&req, buff, n);

    char* tempbuff = NULL;
    uint32_t buff_length = 0;

    rtMessage_ToString(req, &tempbuff, &buff_length);
    rtLog_Debug("Req : %.*s", buff_length, tempbuff);
    free(tempbuff);

    rtMessage_GetString(req, "config", &path);
    rtMessage_GetString(req, "enabled", &enabled);
    rtMessage_GetInt32(req,  "threshold", &doiBitmapThreshold);
    strcpy(doibitmapPath, path);

    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","(%s):%d bitmap:%s,enabled:%s,threshold=%d\n", __FUNCTION__, __LINE__, doibitmapPath, enabled, doiBitmapThreshold);

    if(!strcmp(enabled, "true")) {
        doienabled = true;
    } else {
        doienabled = false;
    }
    hasDOIChanged = true;

    rtMessage_Release(req);
    return;
}

/** @descripion: This function is used to initialise rtMessage
 *  @parameter:
 *       y_addr
 *       width
 *       height
 *  @return:
 *       int 0 for success
 */
int xcvInterface::rtMessageInit()
{
        rtLog_SetLevel(RT_LOG_INFO);
        rtLog_SetOption(rdkLog);
        rtConnection_Create(&connectionSend, "XVISION_SEND", "tcp://127.0.0.1:10001");
	rtConnection_Create(&connectionRecv, "XVISION_RECV", "tcp://127.0.0.1:10001");

	//Adding listener for topics to listen  & Create thread to listen to incoming messages or request
	rtConnection_AddListener(connectionRecv, "RDKC.SMARTTN.STATUS", smtTnOnMessage, NULL);
	//Adding listener for topics to listen  & Create thread to listen to incoming messages or request
	rtConnection_AddListener(connectionRecv, DYNAMIC_LOG_REQ_RES_TOPIC, dynLogOnMessage, connectionRecv);
	rtConnection_AddListener(connectionRecv, "RDKC.DOI_CONF.REFRESH",onMsgDOIConfRefresh, NULL);
#ifdef _ROI_ENABLED_
	rtConnection_AddListener(connectionRecv, "RDKC.ROI.REFRESH",onMsgROIConfRefresh, NULL);
#endif
#if defined(ENABLE_TEST_HARNESS) && defined(_OBJ_DETECTION_)
	rtConnection_AddListener(connectionRecv, "RDKC.TESTHARNESS.DELIVERYDATA", smtTnOnDeliveryData, NULL);
#endif
#ifdef ENABLE_TEST_HARNESS
        rtConnection_AddListener(connectionRecv, "RDKC.TESTHARNESS.CONFIRM", smtTnOnUpload, NULL);
#endif
        if(XCV_SUCCESS != pthread_create(&rtMessageRecvThread, NULL, receive_rtmessage, NULL))
        {
          RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): can't create thread. \n", __FILE__, __LINE__ );
          return XCV_FAILURE;
        }

	pthread_setname_np(rtMessageRecvThread, "xvision_rtMsg");

        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Initialized rtMessage\n",__FUNCTION__, __LINE__);
        t2_event_d("RTM_INFO_Init", 1);
        return RT_OK;
}

/** @descripion: To notify CVR on analytics data via rtMessage if od frame upload is enabled
 *  @parameter:
 *       vaEngineVersion
 *       timestamp
 *       event_type
 *	 motion_level_raw
 *	 motionScore
 *	 boundingBoxXOrd
 * 	 boundingBoxYOrd
 * 	 boundingBoxHeight
 * 	 boundingBoxWidth
 *	 currentTime
 *  @return:
 *       int 0 for success
 */
int xcvInterface::notifyCVR(char *vaEngineVersion, uint64_t timestamp, uint32_t event_type, float motion_level_raw, float motionScore, uint32_t boundingBoxXOrd, uint32_t boundingBoxYOrd, uint32_t boundingBoxHeight, uint32_t boundingBoxWidth,char* curr_time)
{
	std::string s_timestamp = std::to_string(timestamp);
    rtMessage_Create(&m);
	rtMessage_SetString(m,"vaEngineVersion", vaEngineVersion);
	rtMessage_SetString(m, "timestamp", s_timestamp.c_str());
	rtMessage_SetInt32(m, "event_type", event_type);
	rtMessage_SetDouble(m, "motion_level_raw", motion_level_raw);
	rtMessage_SetDouble(m, "motionScore", motionScore);
	rtMessage_SetInt32(m, "boundingBoxXOrd", boundingBoxXOrd);
	rtMessage_SetInt32(m, "boundingBoxYOrd", boundingBoxYOrd);
	rtMessage_SetInt32(m, "boundingBoxHeight", boundingBoxHeight);
	rtMessage_SetInt32(m, "boundingBoxWidth", boundingBoxWidth);
	rtMessage_SetString(m, "currentTime", curr_time);
	/*rtMessage_SetInt32(m, "metadataGenSec", tstamp -> tv_sec);
	rtMessage_SetInt32(m, "metadataGenNsec", tstamp -> tv_sec);*/

	err = rtConnection_SendMessage(connectionSend, m, "RDKC.CVR");
	rtLog_Debug("SendRequest:%s", rtStrError(err));

	if (err != RT_OK)
	{
	    RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error sending msg via rtmessage\n", __FILE__,__LINE__);
	}
        rtMessage_Release(m);
        return RT_OK;
}

#ifdef _OBJ_DETECTION_
#ifdef _ROI_ENABLED_
/** @descripion: To notify Smart Thumbnail on ROI change
 *  @parameter:
 *       roicoords
 *  @return:
 *       int 0 for success
 */
int xcvInterface::notifySmartThumbnail(std::vector<float> roicoords)
{
    rtMessage m;
    rtMessage_Create(&m);

    rtMessage_SetInt32(m,"ROICount", roicoords.size());
    for(int i = 0; i < roicoords.size(); i++) {
        std::string tag = "ROICoord" + std::to_string(i);
        rtMessage_SetDouble(m, tag.c_str(), roicoords[i]);
    }
    err = rtConnection_SendMessage(connectionSend, m, "RDKC.SMARTTN.ROICHANGE");
    rtLog_Debug("SendRequest:%s", rtStrError(err));

    if (err != RT_OK)
    {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error sending msg via rtmessage\n", __FILE__,__LINE__);
    }
    rtMessage_Release(m);

    return RT_OK;
}
#endif

/** @descripion: To notify Smart Thumbnail on DOI change
 *  @parameter:
 *       enabled
 *       doiBitmapFile
 *       threshold
 *  @return:
 *       int 0 for success
 */
int xcvInterface::notifySmartThumbnail(bool enabled, char * doiBitmapFile, int threshold)
{
    rtMessage m;
    rtMessage_Create(&m);

    rtMessage_SetBool(m, "enabled", enabled);
    rtMessage_SetString(m, "doiBitmapFile", doiBitmapFile);
    rtMessage_SetInt32(m, "threshold", threshold);
    err = rtConnection_SendMessage(connectionSend, m, "RDKC.SMARTTN.DOICHANGE");
    rtLog_Debug("SendRequest:%s", rtStrError(err));

    if (err != RT_OK)
    {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error sending msg via rtmessage\n", __FILE__,__LINE__);
    }
    rtMessage_Release(m);

    return RT_OK;
}
#endif

/** @descripion: To notify Smart Thumbnail on analytics data via rtMessage
 *  @parameter:
 *       vaEngineVersion
 *       smInfo
 *  @return:
 *       int 0 for success
 */
int xcvInterface::notifySmartThumbnail(char *vaEngineVersion, SmarttnMetadata *smInfo, int motionFlags)
{
    rtMessage m = SmarttnMetadata::to_rtMessage(smInfo);
    if(!m) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error rtMessage m should not be NULL\n", __FILE__,__LINE__);
        return RT_ERROR;
    }
    rtMessage_SetInt32(m, "motionFlags", motionFlags);

    rtMessage_SetString(m,"vaEngineVersion", vaEngineVersion);
    err = rtConnection_SendMessage(connectionSend, m, "RDKC.SMARTTN.METADATA");
    rtLog_Debug("SendRequest:%s", rtStrError(err));

    if (err != RT_OK)
    {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error sending msg via rtmessage\n", __FILE__,__LINE__);
    }
    rtMessage_Release(m);

    return RT_OK;
}

/** @descripion: To notify Smart Thumbnail on frame(720*1280 yuv) capture via rtMessage
 *  @parameter:
 *      pid
 *      timestamp
 *  @return:
 *       int 0 for success
 */
//int xcvInterface::notifySmartThumbnail(int32_t pid, struct timespec* tstamp)
#if defined(ENABLE_TEST_HARNESS) && defined(_OBJ_DETECTION_)
int xcvInterface::notifySmartThumbnail(int32_t pid, uint64_t timestamp, int fileNum, int frameNum, int fps)
#else
int xcvInterface::notifySmartThumbnail(int32_t pid, uint64_t timestamp)
#endif
{
	std::string s_timestamp = std::to_string(timestamp);
	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VIDEOANALYTICS","%s(%d) framePTS(string):%s\n", __FILE__,__LINE__, s_timestamp.c_str());

//	if(NULL != tstamp) {
		rtMessage_Create(&m);
		rtMessage_SetInt32(m, "processID", pid);
        	rtMessage_SetString(m, "timestamp", s_timestamp.c_str());
#if defined(ENABLE_TEST_HARNESS) && defined(_OBJ_DETECTION_)
                rtMessage_SetInt32(m, "fileNum", fileNum);
                rtMessage_SetInt32(m, "frameNum", frameNum);
                rtMessage_SetInt32(m, "fps", fps);
#endif

		err = rtConnection_SendMessage(connectionSend, m, "RDKC.SMARTTN.CAPTURE");
		rtLog_Debug("SendRequest:%s", rtStrError(err));

		if (err != RT_OK)
		{
			RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error sending msg via rtmessage\n", __FILE__,__LINE__);
        	}
        	rtMessage_Release(m);
//	}
        return RT_OK;
}

#if defined(ENABLE_TEST_HARNESS) && defined(_OBJ_DETECTION_)
int xcvInterface::notifySmartThumbnail_ClipStatus(int32_t status, char *clip_name)
{
        if(!clip_name) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): Empty clip name!!\n", __FILE__, __LINE__);
        }
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d) Sending %s, clipname : %s\n", __FILE__,__LINE__, (status == 0) ? "CLIP_GEN_START" : "CLIP_GEN_END", clip_name);
	rtMessage_Create(&m);
	rtMessage_SetInt32(m, "clipStatus", status);
        rtMessage_SetString(m, "clipname", clip_name);
	err = rtConnection_SendMessage(connectionSend, m, "RDKC.TH.CLIP.STATUS");
	rtLog_Debug("SendRequest:%s", rtStrError(err));

	if (err != RT_OK) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error sending msg via rtmessage\n", __FILE__,__LINE__);
        }
        rtMessage_Release(m);
        return RT_OK;
}

int xcvInterface::notifySmartThumbnail_ClipUpload(char *clip_name)
{
        rtMessage msg;
        int status = 0; //CVR_UPLOAD_OK
        rtMessage_Create(&msg);
        rtMessage_SetInt32(msg, "status", status);
        rtMessage_SetString(msg, "uploadFileName", clip_name);
        RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VIDEOANALYTICS","%s(%d): Sending CVR upload status to smart thumbnail, status code:%d\n",__FILE__, __LINE__, status);
        rtError err = rtConnection_SendMessage(connectionSend, msg, "RDKC.TH.UPLOAD.STATUS");
        rtLog_Debug("SendRequest:%s", rtStrError(err));
        if (err != RT_OK) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error sending msg via rtmessage\n", __FILE__,__LINE__);
        }
        rtMessage_Release(msg);
        return RT_OK;
}
#endif
/** @descripion: To notify CVR on analytics data via rtMessage if od frame upload is disabled
 *  @parameter:
 *      timestamp
 *      event_type
 *      motion_level_raw
 *	current time
 *  @return:
 *       int 0 for success
 */
int xcvInterface::notifyCVR(uint64_t timestamp, uint32_t event_type, float motion_level_raw, char* curr_time)
{
	std::string s_timestamp = std::to_string(timestamp);
	rtMessage_Create(&m);
	rtMessage_SetString(m, "timestamp", s_timestamp.c_str());
        rtMessage_SetInt32(m, "event_type", event_type);
        rtMessage_SetDouble(m, "motion_level_raw", motion_level_raw);
	rtMessage_SetString(m, "currentTime", curr_time);

        err = rtConnection_SendMessage(connectionSend, m, "RDKC.CVR");
        rtLog_Debug("SendRequest:%s", rtStrError(err));

        if (err != RT_OK)
        {
            RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error sending msg via rtmessage\n", __FILE__,__LINE__);
        }
        rtMessage_Release(m);
        return RT_OK;
}

/** @descripion: This function is used to close rtMessage connection
 *  @parameter:
 *       y_addr
 *       width
 *       height
 *  @return:
 *       int 0 for success
 */
int xcvInterface::rtMessageClose()
{
        rtConnection_Destroy(connectionSend);
	rtConnection_Destroy(connectionRecv);
        return RT_OK;
}
#endif

/** @descripion: This function is used to get iva result
 *  @return vai_result_t pointer iva_result
 */
vai_result_t* xcvInterface::get_vai_structure()
{
    if(NULL == vai_result) {
        vai_result_t* iv_result = (vai_result_t*) malloc(sizeof(vai_result_t));
        vai_result = iv_result;
    }

    return vai_result;
}

/** @descripion: This function is used to reset VAI results
 *  @param: void
 *  @return: void
 */
void xcvInterface::reset_vai_results()
{
    if(NULL != vai_result) {
	memset(vai_result, 0, sizeof(vai_result_t));
	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):Resetting VAI Results Structure of size[%d]\n", __FILE__, __LINE__, sizeof(vai_result_t));
    }
    else {
	RDK_LOG( RDK_LOG_WARN,"LOG.RDK.XCV","%s(%d):VAI Results Structure has not allocated yet!!!\n", __FILE__, __LINE__);
    }
    return;

}

/** @descripion: This function is used to convert iav results
 *  @parameter:
 *  framePTS    - unsigned long
 *  vai_results - vai_result_t pointer
 *  objects     - iObject pointer
 *  count       - int
 *  @return: XCV_SUCCESS for success
 */
int xcvInterface::convert_iav_results(unsigned long long framePTS, vai_result_t * vi_results, iObject *objects, int count)
{
    if(NULL == vi_results || NULL == objects) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):input param error : Malloc Error  \n", __FILE__, __LINE__);
        return XCV_FAILURE;
    }
//    vi_results->timestamp = framePTS;
    vi_results->num = count;       //the low 8 bits is the objects number
    vi_results->num |= (iavInterfaceAPI::get_g_input_res()<<8); //the high 8 bits is the raw data resolution index
    vai_object_t *vai_obj = &(vi_results->vai_objects[0]);
    while(count > 0) {
        vai_obj->od_id = objects->m_i_ID;
        if(objects->m_e_class == eOC_Human) {
            vai_obj->type = OD_TYPE_HUMAN;
        }
        else {
            vai_obj->type = OD_TYPE_UNKNOW;
        }
        vai_obj->confidence = objects->m_f_confidence;
        vai_obj->start_x = objects->m_u_bBox.m_i_lCol;
        vai_obj->start_y = objects->m_u_bBox.m_i_tRow;
        vai_obj->width = objects->m_u_bBox.m_i_rCol - objects->m_u_bBox.m_i_lCol;
        vai_obj->height = objects->m_u_bBox.m_i_bRow - objects->m_u_bBox.m_i_tRow;
        count--;
        objects++;
        vai_obj++;
    }
    return XCV_SUCCESS;
}

/** @description: Get Current Time stamp 
 *  @parameter:
 *  curr_time - unsigned int pointer
 *  @return: XCV_SUCCESS for success
 */
int xcvInterface::get_current_time(struct timespec *curr_time)
{
    int result = -1;
    struct timespec tp = {0};
    clockid_t clk_id = CLOCK_REALTIME;

    if(NULL  == curr_time) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tTrying to use invalid memory location.\n",__FILE__,__LINE__);
	return XCV_FAILURE;
    }
	
    clk_id = CLOCK_REALTIME;
    result = clock_gettime(clk_id, &tp);

    if(XCV_SUCCESS != result) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError retriving the current time.\n",__FILE__,__LINE__);
        return XCV_FAILURE;
    }

    curr_time->tv_sec = tp.tv_sec;
    curr_time->tv_nsec = tp.tv_nsec;

    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):\tCurrent time: %lu\n",__FILE__,__LINE__, *curr_time);

    return XCV_SUCCESS;
}


/* @description: This function is used to update the message m with smInfo
 * @parameter: smInfo
 * @return: rtMessage
 */
const rtMessage  SmarttnMetadata::to_rtMessage(const SmarttnMetadata *smInfo ) {

    if(!smInfo) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error smInfo should not be NULL\n", __FILE__,__LINE__);
        return NULL;
    }
    std::string s_timestamp = std::to_string(smInfo->timestamp);

    rtMessage m;
    rtMessage_Create(&m);
    rtMessage_SetString(m, "timestamp", s_timestamp.c_str());
    rtMessage_SetInt32(m, "event_type", smInfo->event_type);
    rtMessage_SetDouble(m, "motionScore", smInfo->motionScore);
    rtMessage_SetString(m, "currentTime", smInfo->s_curr_time);

    //unionBBox
    rtMessage_SetInt32(m, "boundingBoxXOrd", smInfo->unionBox.boundingBoxXOrd);
    rtMessage_SetInt32(m, "boundingBoxYOrd", smInfo->unionBox.boundingBoxYOrd);
    rtMessage_SetInt32(m, "boundingBoxWidth", smInfo->unionBox.boundingBoxWidth);
    rtMessage_SetInt32(m, "boundingBoxHeight", smInfo->unionBox.boundingBoxHeight);

#ifdef _OBJ_DETECTION_
    //unionBBox
    rtMessage_SetInt32(m, "d_boundingBoxXOrd", smInfo->deliveryUnionBox.boundingBoxXOrd);
    rtMessage_SetInt32(m, "d_boundingBoxYOrd", smInfo->deliveryUnionBox.boundingBoxYOrd);
    rtMessage_SetInt32(m, "d_boundingBoxWidth", smInfo->deliveryUnionBox.boundingBoxWidth);
    rtMessage_SetInt32(m, "d_boundingBoxHeight", smInfo->deliveryUnionBox.boundingBoxHeight);
#endif

    //objectBoxs
    for(int32_t i=0; i<UPPER_LIMIT_BLOB_BB; i++)
    {
        if((smInfo->objectBoxs[i].boundingBoxXOrd)  == INVALID_BBOX_ORD)
            break;
        rtMessage bbox;
        rtMessage_Create(&bbox);

        rtMessage_SetInt32(bbox, "boundingBoxXOrd", smInfo->objectBoxs[i].boundingBoxXOrd);
        rtMessage_SetInt32(bbox, "boundingBoxYOrd", smInfo->objectBoxs[i].boundingBoxYOrd);
        rtMessage_SetInt32(bbox, "boundingBoxWidth", smInfo->objectBoxs[i].boundingBoxWidth);
        rtMessage_SetInt32(bbox, "boundingBoxHeight", smInfo->objectBoxs[i].boundingBoxHeight);
        rtMessage_AddMessage(m, "objectBoxs", bbox);
        rtMessage_Release(bbox);
    }

    return m;
}

/* @description: This function is used to update the smInfo
 * @parameter:
 *  engine: xcvAnalyticsEngine
 *  vai_results: vai_result_t structure pointer
 *  curr_time: char pointer of curr_time
 * @return: retrurns SmarttnMetadata pointer
 */
SmarttnMetadata * SmarttnMetadata::from_xcvInterface( xcvAnalyticsEngine* engine, vai_result_t * vai_results, char * curr_time)
{
    if(!(engine && vai_results)) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) engine and vai_results should not be NULL \n", __FILE__,__LINE__);
        return NULL;
    }

    SmarttnMetadata *smInfo = new SmarttnMetadata();
    if(!smInfo) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d) Error creating new SmarttnMetadata \n", __FILE__,__LINE__);
        return NULL;
    }

    smInfo->timestamp = vai_results->timestamp;
    smInfo->event_type = vai_results->event_type;
    smInfo->motionScore = engine ->motionScore;
    smInfo->s_curr_time = curr_time;

#ifdef _OBJ_DETECTION_
    //Delivery - unionBBox
    smInfo->deliveryUnionBox.boundingBoxXOrd = engine  -> deliveryBoundingBoxXOrd;
    smInfo->deliveryUnionBox.boundingBoxYOrd = engine -> deliveryBoundingBoxYOrd;
    smInfo->deliveryUnionBox.boundingBoxWidth = engine -> deliveryBoundingBoxWidth;
    smInfo->deliveryUnionBox.boundingBoxHeight = engine -> deliveryBoundingBoxHeight;
#endif

    //unionBBox
    smInfo->unionBox.boundingBoxXOrd = engine  -> boundingBoxXOrd;
    smInfo->unionBox.boundingBoxYOrd = engine -> boundingBoxYOrd;
    smInfo->unionBox.boundingBoxWidth = engine -> boundingBoxWidth;
    smInfo->unionBox.boundingBoxHeight = engine -> boundingBoxHeight;;

    //objectBBoxs
    for (int32_t i=0, index =0; i< UPPER_LIMIT_BLOB_BB; i++, index += 4) {
        if((engine -> blobBoundingBoxCoords[index])  == INVALID_BBOX_ORD)
            break;
         smInfo->objectBoxs[i].boundingBoxXOrd  = engine -> blobBoundingBoxCoords[index+0];
         smInfo->objectBoxs[i].boundingBoxYOrd  = engine -> blobBoundingBoxCoords[index+1];
         smInfo->objectBoxs[i].boundingBoxWidth  = engine -> blobBoundingBoxCoords[index+2];
         smInfo->objectBoxs[i].boundingBoxHeight  = engine -> blobBoundingBoxCoords[index+3];
         RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): objectBoxs[%d].boundingBoxXOrd:%d\n",__FILE__, __LINE__ ,i,engine->blobBoundingBoxCoords[index+0]);
         RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): objectBoxs[%d].boundingBoxYOrd:%d\n",__FILE__, __LINE__,i,engine->blobBoundingBoxCoords[index+1]);
         RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): objectBoxs[%d].boundingBoxWidth:%d\n",__FILE__, __LINE__,i,engine->blobBoundingBoxCoords[index+2]);
         RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): objectBoxs[%d].boundingBoxHeight:%d\n",__FILE__, __LINE__,i,engine->blobBoundingBoxCoords[index+3]);
    }

    return smInfo;
}

/* @description: SmarttnMetadata constructor
 * @parametar: void
 * @return: void
 */
SmarttnMetadata::SmarttnMetadata()
{
    timestamp = 0;
    event_type = 0;
    motionScore = 0;
    motion_level_raw = 0;
    memset(&unionBox, 0, sizeof(unionBox));
    for( int i=0; i< UPPER_LIMIT_BLOB_BB; i++) {
        objectBoxs[i].boundingBoxXOrd = INVALID_BBOX_ORD;
        objectBoxs[i].boundingBoxYOrd = INVALID_BBOX_ORD;
        objectBoxs[i].boundingBoxWidth = INVALID_BBOX_ORD;
        objectBoxs[i].boundingBoxHeight = INVALID_BBOX_ORD;
    }
    s_curr_time = NULL;
}
