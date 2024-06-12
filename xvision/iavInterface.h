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


#ifndef _IAVINTERFACE_H_
#define _IAVINTERFACE_H_

#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include "VAStructs.h"
#include "va_defines.h"

#ifdef _HAS_XSTREAM_
#include "xStreamerConsumer.h"
#else
#include "RdkCPluginFactory.h"
#include "RdkCVideoCapturer.h"
#endif

#include "telemetry_busmessage_sender.h"

class iavInterfaceAPI
{
   private:
    static int fd_iav;
    static int g_iav_fd;
    static int g_buf_id;
    static int g_input_res;
    static u8 *dsp_mem;
    static unsigned char *g_cur_ydata;
    static unsigned char *g_cur_uvdata;
    static unsigned char *g_cur_me1data;

#ifdef _HAS_XSTREAM_
    static XStreamerConsumer* consumer;
    static frameInfoYUV  *frameInfo;
#ifndef _DIRECT_FRAME_READ_
    static curlInfo frameHandler;
#endif
#else
	//static pluginInterface *interface;
    static RdkCPluginFactory* temp_factory;
    static RdkCVideoCapturer* recorder;
    static RDKC_PLUGIN_YUVInfo *frame;
#endif

   public:
    /* Get milliseconds */
	static int getmsec();
    /* Get aspect ratio */
	static int iav_get_aspect_ratio();
    /* Get fd */
	static int iav_get_fd_iav();
    /* Get buffer id */
	static int iav_get_buf_id();
    /* Initialize source buffer */
        static int rdkc_source_buffer_init(int buf_key_val, int *width, int *height);
    /* Close source buffer */
        static int rdkc_source_buffer_close();
    /* Get YUV frame */
        static int rdkc_get_yuv_frame(unsigned long long *framePTS, iImage *plane0, iImage *plane1);
    /* Get ME1 frame */
        static int rdkc_get_me1_frame(unsigned long long *framePTS, iImage *plane0, iImage *plane1);
    /* Convert iav results */
        //static int convert_iav_results(unsigned long long framePTS, vai_result_t * vai_results, iObject *objects, int count);
    /* Get Day night mode */
#ifdef _HAS_XSTREAM_
	static int read_DN_mode();
#else
	static int read_DN_mode(PLUGIN_DayNightStatus *day_night_status);
#endif
    /* Get g_input_res */
	static int get_g_input_res();
    /* init rdkc_va_send_result*/
	static int VA_send_init();
    /* send va result */
	static int VA_send_result(int id,vai_result_t *va_result);

};
#endif
