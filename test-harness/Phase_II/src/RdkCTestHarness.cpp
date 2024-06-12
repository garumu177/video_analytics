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
#include "RdkCTestHarnessPhaseII.h"

using namespace std;
#ifdef __cplusplus
extern "C" {
#endif

TestHarness *th;

#ifdef TEST_HARNESS_SOCKET
int RdkCTHInit()
{
        if( RDKC_FAILURE == (th -> RdkCTHInit()) ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHInit fails. \n",__FUNCTION__, __LINE__);
		return RDKC_FAILURE;
	}
	return RDKC_SUCCESS;
}

int RdkCTHCheckConnStatus(const char *ifname)
{
	if(RDKC_FAILURE == (th -> RdkCTHCheckConnStatus(ifname))){
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHCheckConnStatus fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
	}
	return RDKC_SUCCESS;
}

int RdkCTHSetVAEngine(const char *VAEngine)
{
        th = new TestHarness;
	if( RDKC_FAILURE == (th -> RdkCTHSetVAEngine(VAEngine)) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHSetVAEngine fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
        return RDKC_SUCCESS;
}

int RdkCTHSetImageResolution(int width, int height)
{
	if( RDKC_FAILURE == (th -> RdkCTHSetImageResolution(width, height)) ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHSetImageResolution fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
	}
	return RDKC_SUCCESS;
}

bool RdkCTHGetFileFeedEnabledParam()
{
	if(strcmp(th -> enableTestHarnessOnFileFeed, "true") == 0) {
		return true;
	}
	else {
		return false;
	}
}

bool RdkCTHGetResetAlgOnFirstFrame()
{
	if(strcmp(th -> resetAlgOnFirstFrame, "true") == 0) {
		return true;
	}
	else {
		return false;
	}
}

int RdkCTHGetFrame(th_iImage *plane0, th_iImage *plane1, int &fps)
{
	int return_val = (th -> RdkCTHGetFrame (plane0, plane1, fps));
        if( RDKC_FAILURE == return_val ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHGetFrame fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
	if( RDKC_FILE_START == return_val ) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHGetFrame starts from new file. \n",__FUNCTION__, __LINE__);
                return RDKC_FILE_START;
	}
	if( RDKC_FILE_END == return_val ) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHGetFrame end of a file. \n",__FUNCTION__, __LINE__);
                return RDKC_FILE_END;
	}
        return RDKC_SUCCESS;
}

int RdkCTHPassFrame(th_iImage *plane0, th_iImage *plane1)
{
        if( RDKC_FAILURE == (th -> RdkCTHPassFrame(plane0,plane1)) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHPassFrame fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
        return RDKC_SUCCESS;
}

int RdkCTHProcess( int objCount, int evtCount, th_iObject *th_objects, th_iEvent *th_events, bool th_isMotionInsideROI, bool th_isMotionInsideDOI)
{
	if( RDKC_FAILURE == (th -> RdkCTHResetPayload()) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHResetPayload fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
	if( RDKC_FAILURE == (th -> RdkCTHCreatePayload(objCount,evtCount,th_objects,th_events, th_isMotionInsideROI, th_isMotionInsideDOI)) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHCreatePayload fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
/*	if( RDKC_FAILURE == (th -> RdkCTHUploadPayload()) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHUploadPayload fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }*/
        return RDKC_SUCCESS;
}

bool RdkCTHIsROICoordinatesChanged()
{
        return th -> RdkCTHIsROICoordinatesChanged();
}

std::string RdkCTHGetROICoordinates()
{
        return th -> RdkCTHGetROICoordinates();
}

bool RdkCTHGetDOIConfig(int &threshold)
{
        return th -> RdkCTHGetDOIConfig(threshold);
}

int RdkCTHWriteFrame(std::string filename)
{
	return th -> RdkCTHWriteFrame(filename);
}

int RdkCTHAddDeliveryResult(th_deliveryResult result)
{
	return th -> RdkCTHAddDeliveryResult(result);
}

int RdkCTHAddSTNUploadStatus(th_smtnUploadStatus stnStatus)
{
        return th->RdkCTHAddSTNUploadStatus(stnStatus);
}

int RdkCTHGetClipSize(int32_t *clip_size)
{
	return th -> RdkCTHGetClipSize(clip_size);
}

int RdkCTHIsDOIChanged()
{
	return th -> RdkCTHIsDOIChanged();
}

int RdkCTHRelease()
{
        if( RDKC_FAILURE == (th -> RdkCTHRelease()) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): RdkCTHRelease fails. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
        delete th;
        th = NULL;
        return RDKC_SUCCESS;
}
#endif
#ifdef __cplusplus
}
#endif
