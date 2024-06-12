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

#include <iostream>
#include "RdkCVAImageTools.h"
extern "C"
{
#include "rdkc_stream_api.h"
}

/**
 * @brief main
 * @param command line argument for compression ratio.
 * @return 0.
 */
int main(int argc, char *argv[])
{
	int ret = -1;
	RDKC_YUVInfo yuv_info;
	yuv_info.buf_id = 3;

	if (-1 == rdkc_raw_init()) {
                printf("Raw init failed!\n");
        }

	ret = rdkc_read_YUV_frame(&yuv_info);

	if (0 == ret) {
		printf("(%d): Got one frame! width=%d, height=%d, pitch=%d, seq_num=%d, format=%d, flag=%d, dsp_pts=%llu, mono_pts=%llu!\n", __LINE__, yuv_info.width, yuv_info.height, yuv_info.pitch, yuv_info.seq_num, yuv_info.format, yuv_info.flag, yuv_info.dsp_pts, yuv_info.mono_pts);
	}
	else if (1 == ret) {
		// No frame data ready, try again
		printf("(%d): No frame data ready, try again.\n", __LINE__);
	}
	else {
		printf("(%d): Read YUV stream error.\n", __LINE__);
	}

	ImageTools imgt;
	std::string snapshot_filename;
	snapshot_filename = "snap.jpeg";
	int compression_scale = atoi(argv[1]);
	int new_width = 640;
	int new_height = 360;

	iImage plane0;
	iImage plane1;
	plane0.data = (unsigned char*) yuv_info.y_addr;
        plane0.size = yuv_info.width*yuv_info.height;
        plane0.height = yuv_info.height;
        plane0.width = yuv_info.width;
        plane1.data =  (unsigned char*)yuv_info.uv_addr;
        plane1.size = yuv_info.width*yuv_info.height;
        plane1.height = yuv_info.height;
        plane1.width = yuv_info.width;

	imgt.RdkCVASnapshot_NV12(&plane0, &plane1, snapshot_filename, compression_scale, new_width, new_height);

	rdkc_raw_close();
	return 0;
}
