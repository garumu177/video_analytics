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
#ifdef BREAKPAD
#include "breakpadwrap.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
#include "dev_config.h"
#ifdef __cplusplus
}
#endif
#define DEFAULT_BUF_ID          0
#define MAX_RETRY	3

void help()
{
	printf("Usage: ./rdkc_snapshooter <snapshot_image_name_with_full_path> <compression_scale> <new_width> <new_height> <optional: ROIcoords>\n");
	return;
}

int main(int argc, char* argv[])
{
	int retry_count = 0;

	if( argc < 5 ) {
		help();
		return RDKC_FAILURE;
	}
	int buf_id = DEFAULT_BUF_ID;
#ifdef BREAKPAD
	sleep(1);
	BreakPadWrapExceptionHandler eh;
	eh = newBreakPadWrapExceptionHandler();
#endif

	if( RDKC_SUCCESS != config_init() ) {
                RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): config_init failed!\n", __FILE__, __LINE__);
                return RDKC_FAILURE;
    	}

        char* configParam = NULL;

        //Get the buffer ID to use
#if defined(XCAM2) ||defined(S5L) || defined(S2L35M)
        configParam = (char*)rdkc_envGet(THUMBNAIL_BUF_ID_2);
#else
        configParam = (char*)rdkc_envGet(THUMBNAIL_BUF_ID_0);
#endif
        if( NULL == configParam ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Error retrieving thumbnail_buf_id from config file. Setting it to default buf id = %d.\n",__FILE__,__LINE__,DEFAULT_BUF_ID);
                buf_id = DEFAULT_BUF_ID;
        }
        else {
                buf_id = atoi(configParam);
        }

	ImageTools *imgt = new ImageTools();
	if( NULL == imgt ) {
		return RDKC_FAILURE;
	}

	std::string roicoords = "";
	std::string snapshot_filename = argv[1]; /* snapshot_image_name_with_full_path */
	int compression_scale = atoi(argv[2]);  /* compression_scale. range 0 - 100 */
	int new_width = atoi(argv[3]);	    /* desired width of the output image */
	int new_height = atoi(argv[4]);	    /* desired height of the output image */

	// Optional parameter to draw ROI - roi coordinates string
	if( argc >= 6 ) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): RdkC Snapshooter with ROI: %s\n",__FUNCTION__, __LINE__, argv[5]);
		roicoords = argv[5];
	}

	do {
		if( RDKC_SUCCESS != imgt->Init(buf_id) ) {
			RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): RdkC Snapshooter Initializationt failed, retry count %d\n",__FUNCTION__, __LINE__, retry_count+1 );
			imgt->Close();
			usleep(10000);
			continue;
		}
		if( RDKC_SUCCESS != imgt->GenerateSnapshot(snapshot_filename, compression_scale, new_width, new_height, roicoords) ) {
			RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): RdkC Snapshooter GenerateSnapshot failed, retry count %d\n", __FUNCTION__, __LINE__, retry_count+1 );
			imgt->Close();
			usleep(10000);
		}
		else {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): RdkC Snapshooter success\n", __FUNCTION__, __LINE__ );
			break;
		}
	} while (++retry_count <= MAX_RETRY);

	if( NULL != imgt ) {
		delete imgt;
		imgt = NULL;
	}
	if( RDKC_SUCCESS != config_release() ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): config_release failed\n",__FILE__, __LINE__);
        }
	return RDKC_SUCCESS;
}
