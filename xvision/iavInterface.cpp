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

#include <sys/time.h>
#include "iavInterface.h"
#include "rdk_debug.h"

#define RDKC_SUCCESS 0
#define RDKC_FAILURE -1

#define ASPECT_RATIO 0
#define BUFFER_ID 3

int iavInterfaceAPI::g_iav_fd;
int iavInterfaceAPI::g_buf_id;
int iavInterfaceAPI::g_input_res;
u8* iavInterfaceAPI::dsp_mem = NULL;

unsigned char* iavInterfaceAPI::g_cur_ydata = NULL;
unsigned char* iavInterfaceAPI::g_cur_uvdata = NULL;
unsigned char* iavInterfaceAPI::g_cur_me1data = NULL;
//pluginInterface* iavInterfaceAPI::interface = new pluginInterface();
int iavInterfaceAPI::fd_iav = -1;

#ifdef _HAS_XSTREAM_
XStreamerConsumer* iavInterfaceAPI::consumer = NULL;
frameInfoYUV* iavInterfaceAPI::frameInfo = NULL;
#ifndef _DIRECT_FRAME_READ_
curlInfo iavInterfaceAPI::frameHandler = {NULL, -1};
#endif
#else
RdkCPluginFactory* iavInterfaceAPI::temp_factory = CreatePluginFactoryInstance(); //creating plugin factory instance
RdkCVideoCapturer* iavInterfaceAPI::recorder = ( RdkCVideoCapturer* )temp_factory->CreateVideoCapturer();
RDKC_PLUGIN_YUVInfo* iavInterfaceAPI::frame = NULL;
#endif

/* engine configuration structure */
typedef struct _iv_config
{
    int sensitivity;
    int people_detection;
    int vehicle_detection;
    int obj_width_min;
    int obj_height_min;
    int obj_width_max;
    int obj_height_max;
} iv_config_t;


/** @descripion: This function is used to return current time in milliseconds
 *  @return int ms
 */
int iavInterfaceAPI::getmsec()
 {
   struct timeval tp;
   gettimeofday(&tp, NULL);
   int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
   return ms;
 }

/** @descripion: This function is used to get aspect ratio
 *  @return int ASPECT_RATIO
 */
int iavInterfaceAPI::iav_get_aspect_ratio()
 {
   return ASPECT_RATIO;
 }

/** @descripion: This function is used to get fd
 *  @return int fd_iav
 */
int iavInterfaceAPI::iav_get_fd_iav()
{
   if(fd_iav < 0) {
     if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
       RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): %s:%s\n", __FILE__, __LINE__,"/dev/iav",strerror(errno));
       return XCV_FAILURE;
      }
    }
   return fd_iav;
}


/** @descripion: This function is used to get buffer id
 *  @return int BUFFER_ID
 */
int iavInterfaceAPI::iav_get_buf_id()
 {
   return BUFFER_ID;
 }

/** @descripion: This function is used to Initialize resource buffer
 *  @parameter:
 *  int  buf_key_val : key value for source buffer
 *  int *width  : source buffer resolution width
 *  int *height : source buffer resolution height
 *  @return XCV_SUCCESS if success,XCV_FAILURE if failure
 */
int iavInterfaceAPI::rdkc_source_buffer_init(int buf_key_val, int *width, int *height)
{
    int ret = RDKC_FAILURE;
    g_buf_id = buf_key_val;

#ifdef _HAS_XSTREAM_
    unsigned int bufferWidth = 0;
    unsigned int bufferHeight = 0;

    //Allocate consumer object
    consumer = new XStreamerConsumer;
    if (NULL == consumer){
       RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):XStreamerConsumer malloc error \n", __FILE__ , __LINE__);
       return XCV_FAILURE;
    }

    //Get the source buffer configuration for the given buffer ID
    ret = consumer->GetSourceBufferResolution(g_buf_id, bufferWidth, bufferHeight );
    if(RDKC_SUCCESS != ret) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):Error Querying Source Buffer \n", __FILE__, __LINE__);
        return XCV_FAILURE;
    }

    //Update the width and height parameter for the engine
    *width = bufferWidth;
    *height = bufferHeight;

    g_cur_ydata = (unsigned char *)malloc(bufferWidth * bufferHeight*2);  //y and uv data
    if (NULL == g_cur_ydata) {
        perror("malloc error!\n");
        return XCV_FAILURE;
    }

    g_cur_uvdata = g_cur_ydata + bufferWidth * bufferHeight;
    memset(g_cur_ydata, 0, bufferWidth * bufferHeight*2);

    //initialize
#ifdef _DIRECT_FRAME_READ_
    ret = consumer->RAWInit((u16)g_buf_id);
    if ( ret < 0 )
#else
    frameHandler = consumer->RAWInit(g_buf_id, FORMAT_YUV, 1);
    if ( frameHandler.sockfd < 0 )
#endif //_DIRECT_FRAME_READ_
    {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.XCV",
                     "%s(%d):Failed to initialize resources to read RAW frames\n", __FILE__,
                     __LINE__ );
            return XCV_FAILURE;
    }

    frameInfo = consumer->GetRAWFrameContainer();

    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV"," %s(%d) Init_iav done: g_cur_ydata:0x%X, width:%d, height:%d\n", __FILE__, __LINE__,g_cur_ydata, *width, *height);

#else
    RDKC_PLUGIN_YUVBufferInfo *buf_format = (RDKC_PLUGIN_YUVBufferInfo *)malloc(sizeof(RDKC_PLUGIN_YUVBufferInfo));
    //if(RDKC_SUCCESS != interface -> query_source_buffer(recorder,g_buf_id , buf_format)) {
    ret = recorder -> GetSourceBufferConfig(g_buf_id,buf_format);
    if(RDKC_SUCCESS != ret) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):Error Querying Source Buffer \n", __FILE__, __LINE__);
        return XCV_FAILURE;
    }

    g_cur_ydata = (unsigned char *)malloc(buf_format->width * buf_format->height*2);  //y and uv data
    if (NULL == g_cur_ydata) {
        perror("malloc error!\n");
        return XCV_FAILURE;
    }

    g_cur_uvdata = g_cur_ydata + buf_format->width * buf_format->height;
    memset(g_cur_ydata, 0, buf_format->width * buf_format->height*2);

    g_cur_me1data = (unsigned char *)malloc(buf_format->width * buf_format->height);  //y and uv data
    if (NULL == g_cur_me1data) {
        perror("malloc error!\n");
        return XCV_FAILURE;
    }

    memset(g_cur_me1data, 0, buf_format->width * buf_format->height);
    *width = buf_format->width;
    *height = buf_format->height;

    if(*width >= 960) {
        g_input_res = OD_RAW_RES_HD;
    }
    else if(*width >= 640) {
        g_input_res = OD_RAW_RES_QHD;
    }
    else {
        g_input_res = OD_RAW_RES_QVGA;
    }

    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV"," %s(%d) Init_iav done: g_cur_ydata:0x%X, width:%d, height:%d\n", __FILE__, __LINE__,g_cur_ydata, *width, *height);

    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):Source Buffer Initialized Successfully \n", __FILE__ , __LINE__);

    frame = (RDKC_PLUGIN_YUVInfo *) malloc(sizeof(RDKC_PLUGIN_YUVInfo));
    if (NULL == frame) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):Frame malloc error \n", __FILE__ , __LINE__);
        return XCV_FAILURE;
    }
#endif

    return XCV_SUCCESS;
}

/** @descripion: This function is used to close resource buffer
 *  @return int 0 success
 */
int iavInterfaceAPI::rdkc_source_buffer_close()
{
	int ret = XCV_SUCCESS;

#ifdef _HAS_XSTREAM_

#ifdef _DIRECT_FRAME_READ_
    if( RDKC_FAILURE != consumer->RAWClose() )
#else
    //Check if the socket connection has established. If so, then do RAWClose to close the connection
    if( NULL != frameHandler.curl_handle && 0 > frameHandler.sockfd ){
        //Release the curl handle and close the socket
        if( XCV_FAILURE != consumer->RAWClose(frameHandler.curl_handle))
#endif //_DIRECT_FRAME_READ_
        {
            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.SMARTTHUMBNAIL", "%s(%d) RAWClose Successful!!!\n", __FUNCTION__ , __LINE__);
        }
        else{
            RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SMARTTHUMBNAIL", "%s(%d) RAWClose Failed!!!\n", __FUNCTION__ , __LINE__);
            ret = XCV_FAILURE;
        }
#ifndef _DIRECT_FRAME_READ_
    }
#endif //_DIRECT_FRAME_READ_

    //Delete the XStreamerConsumer
    if(NULL != consumer){
        delete consumer;
        consumer = NULL;
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.SMARTTHUMBNAIL", "%s(%d) StreamClose Successful!!!\n", __FUNCTION__ , __LINE__);
    }

	frameInfo = NULL;
#ifndef _DIRECT_FRAME_READ_
	frameHandler.curl_handle = NULL;
	frameHandler.sockfd = -1;
#endif
#else
    if(NULL != frame) {
        free(frame);
        frame = NULL;
    }
#endif
    if(NULL != g_cur_ydata) {
        free(g_cur_ydata);
        g_cur_ydata = NULL;
    }

    return ret;
}

/** @descripion: This function is used to get yuv frame
 *  @parameter:
 *  framePTS- unsigned long pointer
 *  plane0 - iImage pointer plane0
 *  plane1 - iImage pointer plane1
 *  @return XCV_SUCCESS if success,XCV_FAILURE if failure
 */
int iavInterfaceAPI::rdkc_get_yuv_frame(unsigned long long *framePTS, iImage *plane0, iImage *plane1)
{

    int ret = RDKC_FAILURE;

    if(NULL == plane0 || NULL == plane1) {
        perror("input param error: p_vai_raw is NULL!!\n");
        return XCV_FAILURE;
    }

#ifdef _HAS_XSTREAM_
	if(NULL == consumer) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Consumer Instance is NULL  \n", __FILE__, __LINE__);
		return XCV_FAILURE;
	}

    //Read the YUV frame and updates the YUVframe
#ifdef _DIRECT_FRAME_READ_
    ret = consumer->ReadRAWFrame((u16)g_buf_id, (u16)FORMAT_YUV, frameInfo);
#else
    ret = consumer->ReadRAWFrame(g_buf_id, FORMAT_YUV, frameInfo);
#endif //_DIRECT_FRAME_READ_
	if(XSTREAMER_SUCCESS != ret) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):Error in reading YUV Frame  \n", __FILE__, __LINE__);
		t2_event_d("XCV_ERR_ReadFrame", 1);
		if(XSTREAMER_FRAME_NOT_READY == ret) {
			return XCV_OTHER;
		}
		else {
			return XCV_FAILURE;
		}
	}

	if( (NULL != frameInfo) && (NULL != frameInfo->y_addr) && (NULL != frameInfo->uv_addr) ) {
		*framePTS = (unsigned long long )frameInfo ->mono_pts;

		memcpy(g_cur_ydata, frameInfo->y_addr, frameInfo->width * frameInfo->height);
		memcpy(g_cur_uvdata, frameInfo->uv_addr,frameInfo->width * frameInfo->height/2);

		//Update the image pointer for YADDR
		plane0->data = (unsigned char *)g_cur_ydata;
		plane0->size = frameInfo->width * frameInfo->height;
		plane0->width = frameInfo->width;
		plane0->height = frameInfo->height;
		plane0->step = frameInfo->pitch;

		//Update the image pointer for UVADDR
		plane1->data = (unsigned char *)g_cur_uvdata;
		plane1->size = frameInfo->width * frameInfo->height/2;
		plane1->width = frameInfo->width;
		plane1->height = frameInfo->height;
		plane1->step = frameInfo->pitch;
	}

#else
    if (NULL == frame) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):YUV Frame malloc error \n", __FILE__ , __LINE__);
        return XCV_FAILURE;
    }
    memset(frame, 0, sizeof(RDKC_PLUGIN_YUVInfo));

    //if(RDKC_SUCCESS != interface -> readYUVFrame(recorder,g_buf_id,frame)) {
    ret = recorder -> ReadYUVData(g_buf_id,frame);
    if( RDKC_SUCCESS != ret) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):Error REading YUV Frame  \n", __FILE__, __LINE__);
        return XCV_FAILURE;
    }

    *framePTS = (unsigned long long )frame -> mono_pts;
    memcpy(g_cur_ydata, frame->y_addr, frame->width * frame->height);
    memcpy(g_cur_uvdata, frame->uv_addr,frame->width * frame->height/2);

    //gettimeofday(&tv, (struct timezone *)NULL);
    plane0->data = (unsigned char *)g_cur_ydata;
    plane0->size = frame->width * frame->height;
    plane0->width = frame->width;
    plane0->height = frame->height;
    plane0->step = frame->pitch;

    plane1->data = (unsigned char *)g_cur_uvdata;
    plane1->size = frame->width * frame->height/2;
    plane1->width = frame->width;
    plane1->height = frame->height;
    plane1->step = frame->pitch;

#endif

    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d) got input, frame:%u \n", __FILE__ , __LINE__, *framePTS);
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d) plane0: data=0x%X, size=%u, width:%u, height:%u, step:%u\n", __FILE__ , __LINE__, plane0->data, plane0->size, plane0->width, plane0->height, plane0->step);
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d) plane1: data=0x%X, size=%u, width:%u, height:%u, step:%u\n", __FILE__ , __LINE__, plane1->data, plane1->size, plane1->width, plane1->height, plane1->step);

    return XCV_SUCCESS;
}

/** @descripion: This function is used to get me1 frame
 *  @parameter:
 *  framePTS- unsigned long pointer
 *  plane0 - iImage pointer plane0
 *  plane1 - iImage pointer plane1
 *  @return int 0 success
 */
int iavInterfaceAPI::rdkc_get_me1_frame(unsigned long long *framePTS, iImage *plane0, iImage *plane1)
{
    int ret = RDKC_FAILURE;

#ifdef _HAS_XSTREAM_
    //Do nothing
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):ME Support is not available in xStreamer \n", __FILE__, __LINE__);
	return ret;

#else
    if(NULL == frame) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):ME Frame Malloc Error  \n", __FILE__, __LINE__);
        return XCV_FAILURE;
    }
    memset(frame, 0, sizeof(RDKC_PLUGIN_YUVInfo));

    if(NULL == plane0) {
        perror("input param error: p_vai_raw is NULL!!\n");
        return XCV_FAILURE;
    }

    //if (RDKC_SUCCESS != interface -> readMEFrame(recorder,g_buf_id,frame)) {
    ret = recorder -> ReadMEData(g_buf_id,frame);
    if( RDKC_SUCCESS != ret) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):Error REading ME1 Frame  \n", __FILE__, __LINE__);
        return XCV_FAILURE;
    }

    memcpy(g_cur_me1data, frame->y_addr, frame->pitch * frame->height);
    plane0->data = (unsigned char *)g_cur_me1data;
    plane0->size = frame->pitch * frame->height;
    plane0->width = frame->width;
    plane0->height = frame->height;
    plane0->step = frame->pitch;
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d) got input, frame:%u \n", __FILE__ , __LINE__, *framePTS);
    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d) plane0: data=0x%X, size=%u, width:%u, height:%u, step:%u\n", __FILE__ , __LINE__, plane0->data, plane0->size, plane0->width, plane0->height, plane0->step);

    return XCV_SUCCESS;
#endif
}
#if 0
/** @descripion: This function is used to convert iav results
 *  @parameter:
 *  framePTS    - unsigned long
 *  vai_results - vai_result_t pointer
 *  objects     - iObject pointer
 *  count       - int
 *  @return: XCV_SUCCESS for success
 */
int iavInterfaceAPI::convert_iav_results(unsigned long long framePTS, vai_result_t * vai_results, iObject *objects, int count)
{
    if(NULL == vai_results || NULL == objects) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):input param error : Malloc Error  \n", __FILE__, __LINE__);
        return XCV_FAILURE;
    }
    vai_results->timestamp = framePTS;
    vai_results->num = count;       //the low 8 bits is the objects number
    vai_results->num |= (g_input_res<<8); //the high 8 bits is the raw data resolution index
    vai_object_t *vai_obj = &(vai_results->vai_objects[0]);
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
#endif

#ifdef _HAS_XSTREAM_
/** @descripion: This function is used get Day Night Mode
 *  @parameter: Nil
 *  @return: day night mode on success , XCV_FAILURE on failure
 */
int iavInterfaceAPI::read_DN_mode()
{
	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.XCV","%s(%d):Day/Night mode support is not available in xStreamer, hence returning day mode always\n", __FILE__, __LINE__);
    return DAY_MODE; //Returning default mode i.e DAY_MODE
}
#else
/** @descripion: This function is used get Day Night Mode
 *  @parameter:
 *  day_night_status : PLUGIN_DayNightStatus structure
 *  @return: day night mode on success , XCV_FAILURE on failure
 */
int iavInterfaceAPI::read_DN_mode(PLUGIN_DayNightStatus *day_night_status)
{
    int mode = DAY_MODE;

    if (RDKC_FAILURE == recorder -> ReadDNMode(day_night_status))
    {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","Get Day/Night mode info error!\n");
        return XCV_FAILURE;
    }

    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","Current mode is %s_mode!\n",day_night_status -> day_night_flag?"night":"day");
    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","count_time is %d!\n",day_night_status -> count_time);
    mode = day_night_status -> day_night_flag?NIGHT_MODE:DAY_MODE;

    return mode;
}
#endif

/** @descripion: Function to get Alg Index
 *  @return int
 */
int iavInterfaceAPI::get_g_input_res()
{
    return g_input_res;
}

/** @descripion: Init va send
 *  @return RDKC_FAILURE on failure, va_send_id on success
 */
int iavInterfaceAPI::VA_send_init()
{
    int id = 0;
#ifdef _HAS_XSTREAM_
    //Do nothing

#else
    id = recorder -> RdkcVASendInit();
    if (RDKC_FAILURE == id)
    {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","rdkc_va_send_init error, va_send_id=%d\n",id);
        return XCV_FAILURE;
    }

#endif
    return id;
}
/** @descripion: This function is used to send va_result
 *  @parameter:
 *  id : va_send_result id
 *  va_result : vai_result_t structure
*/
int iavInterfaceAPI::VA_send_result(int id,vai_result_t *va_result)
{
#ifdef _HAS_XSTREAM_
    return XCV_SUCCESS;

#else
    recorder -> RdkcVASendResult(id,va_result);

#endif
}
