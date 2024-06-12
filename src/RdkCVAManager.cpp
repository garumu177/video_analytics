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

#include "RdkCVAManager.h"
#include "RdkCVideoAnalytics.h"

static VideoAnalytics *VA = NULL;

/** @descripion: Set VA Algorithm
 *  @param[in] alg - algorithm to be set
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure.
 */
int RdkCVASetAlgorithm(int alg)
{
	if( VA_FAILURE == VideoAnalytics::RdkCVASetAlgorithm(alg) ){
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVASetAlgorithm fails!",__FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	return VA_SUCCESS;
}

/** @descripion: Initialise VA algorithm
 *  @param: void
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAInit()
{
	VA = new VideoAnalytics();
	if( NULL == VA ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): Can not initialise VideoAnalytics. VA pointer is NULL\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	if( VA_SUCCESS != VA -> RdkCVAInit() ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): Can not initialise VideoAnalytics. RdkCVAInit fails.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	return VA_SUCCESS;
}

/** @descripion: VA process frame
 *  @param[in] data - frame pointer
 *  @param[in] size - frame size
 *  @param[in] height - frame height
 *  @param[in] width - frame width
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAProcessFrame( unsigned char *data, int size, int height, int width )
{
	int ret = VA_FAILURE;
	float val = RDKC_DISABLE;
	if( NULL != VA ) {
		ret = ( VA -> RdkCVAGetProperty(RDKC_PROP_OBJECT_ENABLED, val) );
		if( (VA_SUCCESS == ret) && (RDKC_ENABLE == val) ) {
			if( VA_FAILURE == VA -> RdkCVAProcessFrame(data,size,height,width) ){
				RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAProcessFrame fails.\n", __FUNCTION__, __LINE__);
				return VA_FAILURE;
			}
		}
		else {
			RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): Object detection is not enabled.\n", __FUNCTION__, __LINE__);
			return VA_FAILURE;
		}
	}
	else {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	return VA_SUCCESS;
}

/** @description: Get total blob area for current frame.
 *  @param[in]  blob_threshold : filter to select the blob areas for calculating 
				 total blob area for current frame.
 *  @param[out] currentBlobArea : output parameter for total blob area for 
				  current frame. 
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int  RdkCGetCurrentBlobArea(double blob_threshold, double *currentBlobArea)
{
	if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

	if( VA_FAILURE == VA -> RdkCGetCurrentBlobArea(blob_threshold, currentBlobArea) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCGetCurrentBlobArea fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
	return VA_SUCCESS;
}

/** @description: Get Object count for frame.
 *  @param[out] ObjCount : output parameter for number of objects detected
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetObjectCount(int* ObjCount)
{
	if( NULL == VA ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	memset( (int*) ObjCount, 0, sizeof(int));
	*ObjCount += VA -> RdkCVAGetObjectCount();
	return VA_SUCCESS;
}

/** @description: InIt function for video-analytics module.
 *  @param[out] refObj: Structure object for which memory should be allocated by the application who is calling it.
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetObject(iObject *refObj)
{
	if( NULL == VA ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	memset( (iObject*) refObj, 0, sizeof(iObject));
	if( VA_FAILURE == VA -> RdkCVAGetObject(refObj) ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAGetObject fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
	}
	return VA_SUCCESS;
}

/** @description: Get Object count for frame.
 *  @param[out] EvtCount : output parameter for number of events detected
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetEventCount(int* EvtCount)
{
	if( NULL == VA ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	memset( (int*) EvtCount, 0, sizeof(int));
	*EvtCount += VA -> RdkCVAGetEventCount();
	return VA_SUCCESS;
}

/** @description: InIt function for video-analytics module.
 *  @param[out] refObj: Structure Event for which memory has to
 *                 allocate by the application who is calling this function.
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetEvent(iEvent *refEvent)
{
	if( NULL == VA ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	memset( (iEvent*) refEvent, 0, sizeof(iEvent));
	if( VA_FAILURE == VA -> RdkCVAGetEvent(refEvent) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAGetEvent fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
	return VA_SUCCESS;
}

/** @description: Get property.
 *  @param[in] PropID: Property Id to be get
 *  @param[out] val: Value for that PropID
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetProperty(int PropID, float *val)
{

	if( NULL == VA ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	if( VA_FAILURE == VA -> RdkCVAGetProperty(PropID,*val) ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAGetProperty fails.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	return VA_SUCCESS;
}
/** @description: Set property.
 *  @param[in] PropID: Property Id to be set
 *  @param[in] val: Value to be set for that PropID
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVASetProperty(int PropID, float val)
{

	if( NULL == VA ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	if( VA_FAILURE == VA -> RdkCVASetProperty(PropID,val) ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVASetProperty fails.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	return VA_SUCCESS;
}

/** @description: Reset all video-analytics components.
 *  @param: void
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAResetAlgorithm()
{
	if( NULL == VA ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	if( VA_FAILURE == VA -> RdkCVAResetAlgorithm() ){
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAResetAlgorithm fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
	}
	return VA_SUCCESS;
}
/** @description: Release all the occupied memory.
 *  @param: void
 *  @return: void
 */
void RdkCVARelease()
{
	if( NULL != VA ) {
                VA -> RdkCVARelease();
		delete VA;
		VA = NULL;
	}
	else {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
	}
	return;
}

/** @description: Get XCV algorithm version number.
 *  @param[in]  version_str : version string 
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetXCVVersion(char *version_str, int& versionlength)
{
	if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

	if( VA_FAILURE == VA -> RdkCVAGetXCVVersion(version_str,versionlength) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAGetXCVVersion fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
	return VA_SUCCESS;
}

/** @description: Get motion score for frame.
 *  @param[in]  mode : choose the mode for calculating motion score 
 *  @return: motion score value as a float
 */
float  RdkCVAGetMotionScore(int mode)
{
	if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

	return VA -> RdkCVAGetMotionScore(mode);
}

#ifdef _OBJ_DETECTION_
/** @description: Get Object bounding box coordinates.
 *  @param[in]  bbox_coords : pointer to box coordinates
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetDeliveryObjectBBox(short *bbox_coords)
{
       if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

       if( VA_FAILURE == VA -> RdkCVAGetDeliveryObjectBBox(bbox_coords) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAGetDeliveryObjectBBox fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
       return VA_SUCCESS;
}

int RdkCVASetDeliveryUpscaleFactor(float scaleFactor)
{
       if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

       if( VA_FAILURE == VA -> RdkCVASetDeliveryUpscaleFactor(scaleFactor) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVASetDeliveryUpscaleFactor fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
       return VA_SUCCESS;
}
#endif

int RdkCVASetDOIOverlapThreshold(float threshold)
{
       if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

       if( VA_FAILURE == VA -> RdkCVASetDOIOverlapThreshold(threshold) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVASetDOIOverlapThreshold fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
       return VA_SUCCESS;
}
/** @description: Get Object bounding box coordinates.
 *  @param[in]  bbox_coords : pointer to box coordinates
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetObjectBBoxCoords(short *bbox_coords)
{
	if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

	if( VA_FAILURE == VA -> RdkCVAGetObjectBBoxCoords(bbox_coords) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAGetObjectBBoxCoords fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
	return VA_SUCCESS;
}

/** @description: Get bounding box coordinates of individual blobs.
 *  @param[in]  bboxs : pointer to box coordinates
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAGetBlobsBBoxCoords(short *bboxs)
{
	if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

	if( VA_FAILURE == VA -> RdkCVAGetBlobsBBoxCoords(bboxs) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAGetBlobsBBoxCoords fails.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
	return VA_SUCCESS;
}

#ifdef _ROI_ENABLED_

/** @description: Set ROI.
 *  @param[in] coords: coordinates list
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVASetROI(std::vector<float> coords)
{
        if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

        if( VA_FAILURE == VA -> RdkCVASetROI(coords) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVASetROI failed.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
        return VA_SUCCESS;
}

/** @description: Clear ROI.
 *  @param[in]:
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int RdkCVAClearROI()
{
        if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }

        if( VA_FAILURE == VA -> RdkCVAClearROI() ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): RdkCVAClearROI failed.\n", __FUNCTION__, __LINE__);
                return VA_FAILURE;
        }
        return VA_SUCCESS;
}

/** @description: Check if motion is inside ROI.
 *  @param[in]:
 *  @return: true if motion is inside ROI, else false.
 */
bool RdkCVAIsMotionInsideROI()
{
        if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return false;
        }

        return VA -> RdkCVAIsMotionInsideROI();
}
#endif

/** @description: Apply DOI.
 *  @param[in]:
 *  @return: true on success, else false.
 */
bool RdkCVAapplyDOIthreshold(bool enable, char *doi_path, int doi_threshold)
{
        if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return false;
        }

        return VA -> RdkCVAapplyDOIthreshold(enable, doi_path, doi_threshold);
}

/** @description: Check if motion is inside DOI.
 *  @param[in]:
 *  @return: true if motion is inside DOI, else false.
 */
bool RdkCVAIsMotionInsideDOI()
{
        if( NULL == VA ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): VideoAnalytics is not initialised.\n", __FUNCTION__, __LINE__);
                return false;
        }

        return VA -> RdkCVAIsMotionInsideDOI();
}

