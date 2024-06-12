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

#ifndef __IMAGETOOLS_H_
#define __IMAGETOOLS_H_

/*************************       INCLUDES         *************************/
/* common include */
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <errno.h>
#include "rdk_debug.h"
#ifdef _HAS_XSTREAM_
#include "xStreamerConsumer.h"
#else
#include "RdkCPluginFactory.h"
#include "RdkCVideoCapturer.h"
#endif
#define RDKC_SUCCESS		0
#define RDKC_FAILURE		-1
//#define SNAPSHOT_FILE           "/opt/tn_snapshot.jpg"

struct Frame
{
        unsigned char* y_data;
        unsigned char* uv_data;
        int size;
        int height;
        int width;
};

class ImageTools
{
public:
	/* Constructor */
	ImageTools();
	/* Destructor */
	~ImageTools();
	/* Initialize buffer */
	int Init(int bufferId);
	/* Generate Snapshot */
	int GenerateSnapshot(std::string snapshot_filename, int compression_scale, int new_width, int new_height, std::string roi = "");
	/* Close */
	int Close();
private:
	static unsigned char * yuvSnapshotData;
	Frame *frame;
	int buf_id;

#ifdef _HAS_XSTREAM_
	XStreamerConsumer *consumer;
#ifndef _DIRECT_FRAME_READ_
	curlInfo frameHandler;
#endif
	frameInfoYUV  *frameInfo;
#else
	static RdkCPluginFactory* temp_factory;
	static RdkCVideoCapturer* recorder;
#endif

	/* Get the YUV frame */
	//int GetYUVFrame(iImage* plane0, iImage* plane1);
	int GetYUVFrame();
	/* Generate jpeg image using opencv */
	int RdkCVASnapshot_NV12(std::string snapshot_filename, int compression_scale, int new_width, int new_height, std::string roi = "");
	int RdkCVASnapshot_YV12(std::string snapshot_filename, int compression_scale, int new_width, int new_height);
};

#endif /*#ifndef __IMAGETOOLS_H_ */
