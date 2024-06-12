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
#include <numeric>
#include <algorithm>
#include <functional>
#include <unordered_set>
#include "RdkCVAManager.h"
#include "RdkCVideoAnalytics.h"
#include "RdkCVACommon.h"

//Barry: This is the correct value for the FOV (change in pixel size) scale factor
#if defined(XCAM2) || defined(XCAM3)
#define FOV_SCALE_FACTOR      0.49
#elif defined(DBC)
#define FOV_SCALE_FACTOR      0.65
#else
#define FOV_SCALE_FACTOR      1.00
#endif
#define FOV_SCALE_FACTOR_SQR  ((FOV_SCALE_FACTOR)*(FOV_SCALE_FACTOR))

#if defined(XCAM2) || defined(XCAM3)
#define VARIANCE_MULTIPLIER   125.0
#else
#define VARIANCE_MULTIPLIER   95.0
#endif

#if defined(DBC)
#define DEFAULT_UPSCALE_WIDTH	1280
#define DEFAULT_UPSCALE_HEIGHT	960
#else
#define DEFAULT_UPSCALE_WIDTH	1280
#define DEFAULT_UPSCALE_HEIGHT	720
#endif

#define DOI_OVERLAP_THRESHOLD 0

int VideoAnalytics::va_alg = DEFAULT_VA_ALG;
float activeTimeThreshold = 0;
float varianceThreshold = 0;

/* Constructor */
VideoAnalytics::VideoAnalytics():firstFrame(true), \
				is_ObjectDetected(false), \
				is_MotionDetected(false), \
				numOfObjectsDetected(DEFAULT_OD), \
				numberOfEventsDetected(DEFAULT_EVT), \
				bgs(NULL), \
				md_object(NULL), \
				md_obj_ptr(NULL), \
				md_evt(NULL), \
				scaleFactor(DEFAULT_SCALE_FACTOR), \
				motionLevelPercentage(DEFAULT_MOTION_LEVEL_PERCENTAGE), \
				motionDetection_Enable(RDKC_ENABLE), \
				objectDetection_Enable(RDKC_ENABLE), \
				humanDetection_Enable(RDKC_DISABLE), \
				tamperDetection_Enable(RDKC_DISABLE), \
				sensitivity(DEFAULT_SENSITIVITY), \
				noOfPixelsInMotion(0.0), \
				frameArea(0), \
				upscale_width(DEFAULT_UPSCALE_WIDTH), \
				upscale_height(DEFAULT_UPSCALE_HEIGHT), \
				doiOverlapThreshold(DOI_OVERLAP_THRESHOLD)
{

#ifdef _ROI_ENABLED_
	is_MotionInROI = false;
	roiOverlapThresh = DEFAULT_XCV_ROI_OVERLAP_PERCENTAGE;
	// set motion thresholds
	activeTimeThreshold = 1.0 / sensitivity * LOWER_LIMIT_MAXACTIVETIME;
	varianceThreshold = (1.0 / sensitivity) * (1.0 / sensitivity) * (img_input.cols * img_input.rows / 320.0 / 200.0) * VARIANCE_MULTIPLIER * FOV_SCALE_FACTOR;
	// define lambda that the history will use to determine whether a track is valid (could have triggered motion)
	std::function<bool(const cvb::CvTrack&)> checker([&](const cvb::CvTrack& track){ return isTrackValid(track, activeTimeThreshold, UPPER_LIMIT_MAXACTIVETIME, varianceThreshold); });
	auto inactiveThresh = 12;  // 2 seconds default
	auto maxTracksThresh = 90; // 15 seconds default
	// define the history that will store tracked blobs over time
	history = TrackHistory(
		checker,
		inactiveThresh, // track inactivity threshold after which tracks will be removed
		maxTracksThresh  // max history per track threshold after which tracks will be removed
	);
#endif

        md_object = (iObject *)(malloc((sizeof(iObject)) * RDKC_OD_MAX_NUM));
        if(NULL == md_object) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): malloc error for md_object",__FUNCTION__, __LINE__);
        }
        else {
                md_obj_ptr = md_object;
        }

        md_evt = new iEvent;
        if(NULL == md_evt) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): malloc error for md_evt",__FUNCTION__, __LINE__);
        }

	memset(uboxCoords, 0, sizeof(uboxCoords));
       for (size_t i = 0; i < 4 * UPPER_LIMIT_BLOB_BBS; ++i) {
           blobBBoxCoords[i] = INVALID_BBOX_ORD;
       }
#ifdef _OBJ_DETECTION_
       memset(detectionUboxCoords, 0, sizeof(detectionUboxCoords));
#endif
}

/* Destructor */
VideoAnalytics::~VideoAnalytics()
{
	RdkCVARelease();
        img_mask.release();
        img_bkgmodel.release();
        img_input.release();
	img_output.release();
}

/** @descripion: Set VA Algorithm
 *  @param[in] alg - algorithm to be set
 *  @return: VA_SUCCESS on success.
 */
int VideoAnalytics::RdkCVASetAlgorithm(int alg = DEFAULT_VA_ALG)
{
	if( alg < VA_ALG_GMM || alg > VA_ALG_LBASOM ) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Inavalid algorithm to be set. Setting it to default GMM.",__FUNCTION__, __LINE__);
		va_alg = DEFAULT_VA_ALG;
		return VA_SUCCESS;
	}
	va_alg = alg;
	return VA_SUCCESS;
}

/** @descripion: Set property
 *  @param[in] PropID - property to be set
 *  @param[in] val - property value to be set
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVASetProperty(int PropID, float& val)
{
	if( RDKC_PROP_SCALE_FACTOR == PropID ){
		scaleFactor = val;
		return VA_SUCCESS;
	}
	else if( RDKC_PROP_HUMAN_ENABLED == PropID ){
		if( val != RDKC_ENABLE && val != RDKC_DISABLE ) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Inavalid argument to be set. Setting it to default disable.",__FUNCTION__, __LINE__);
			humanDetection_Enable = (float)RDKC_DISABLE;
                        return VA_SUCCESS;

		}
		humanDetection_Enable = val;
		return VA_SUCCESS;
	}
	else if( RDKC_PROP_SCENE_ENABLED == PropID ){
		if( val != RDKC_ENABLE && val != RDKC_DISABLE) {
                        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Inavalid argument to be set. Setting it to default disable.",__FUNCTION__, __LINE__);
                        tamperDetection_Enable = (float)RDKC_DISABLE;
                        return VA_SUCCESS;

                }
		tamperDetection_Enable = val;
		return VA_SUCCESS;
	}
	else if( RDKC_PROP_OBJECT_ENABLED == PropID ){
		if( val != RDKC_ENABLE && val != RDKC_DISABLE) {
                        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Inavalid argument to be set. Setting it to default enable.",__FUNCTION__, __LINE__);
                        objectDetection_Enable = (float)RDKC_ENABLE;
                        return VA_SUCCESS;

                }
		objectDetection_Enable = val;
		return VA_SUCCESS;
	}
	else if( RDKC_PROP_MOTION_ENABLED == PropID ){
		if( val != RDKC_ENABLE && val != RDKC_DISABLE) {
                        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Inavalid argument to be set. Setting it to default enable.",__FUNCTION__, __LINE__);
                        motionDetection_Enable = (float)RDKC_ENABLE;
                        return VA_SUCCESS;

                }
                motionDetection_Enable = val;
		return VA_SUCCESS;
        }
	else if( RDKC_PROP_ALGORITHM == PropID ){
		if( val < VA_ALG_GMM || val > VA_ALG_LBASOM ) {
                	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Inavalid algorithm to be set. Setting it to default GMM.",__FUNCTION__, __LINE__);
                	va_alg = DEFAULT_VA_ALG;
	                return VA_SUCCESS;
        	}
		va_alg = (int)val;
		return VA_SUCCESS;
	}
	else if( RDKC_PROP_SENSITIVITY == PropID ) {
		if( val <= XCV_MIN_SENSITIVITY || val > XCV_MAX_SENSITIVITY) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Inavalid sensitivity to be set. retaining the existing value %d.",__FUNCTION__, __LINE__,sensitivity);
			return VA_SUCCESS;
		}
		sensitivity = (int)val;
		return VA_SUCCESS;
	}
	else if( RDKC_PROP_UPSCALE_RESOLUTION == PropID ) {
		if( val < UPSCALE_RESOLUTION_FIRST || val > UPSCALE_RESOLUTION_LAST ) {
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VIDEOANALYTICS", "%s(%d): Inavalid upscale resolution, retaining default value\n", __FUNCTION__, __LINE__);
			return VA_SUCCESS;
		}
		/* Setting the resolution to upscale the blobs */
		upscale_resolution = (eRdkCUpScaleResolution_t)val;

		/* Setting the upscaling width and height according to the resolution */
		switch((int)val) {
			case UPSCALE_RESOLUTION_1280_720:
				upscale_width = 1280.0;
				upscale_height = 720.0;
				break;
			case UPSCALE_RESOLUTION_1280_960:
				upscale_width = 1280.0;
				upscale_height = 960.0;
				break;
                        case UPSCALE_RESOLUTION_640_480:
                                upscale_width = 640.0;
                                upscale_height = 480.0;
                                break;
		}
		return VA_SUCCESS;
	}
	return VA_FAILURE;
}

/** @descripion: Get property
 *  @param[in] PropID - property to be get
 *  @param[out] val - property value to be returned
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAGetProperty(int PropID, float& val)
{
	if( RDKC_PROP_MOTION_LEVEL == PropID ){
                val = RdkCVAGetMotionLevel();
		return VA_SUCCESS;
        }
	if( RDKC_PROP_MOTION_LEVEL_RAW == PropID ){
                val = RdkCVAGetRawMotionLevel();
                return VA_SUCCESS;
        }
        else if( RDKC_PROP_SCALE_FACTOR == PropID ){
                val = (float)scaleFactor;
                return VA_SUCCESS;
        }
        else if( RDKC_PROP_OBJECT_ENABLED == PropID ){
                val = objectDetection_Enable;
		return VA_SUCCESS;
        }
        else if( RDKC_PROP_MOTION_ENABLED == PropID ){
                val = motionDetection_Enable;
		return VA_SUCCESS;
        }
	else if( RDKC_PROP_HUMAN_ENABLED == PropID ){
                val = humanDetection_Enable;
		return VA_SUCCESS;
        }
        else if( RDKC_PROP_SCENE_ENABLED == PropID ){
                val = tamperDetection_Enable;
		return VA_SUCCESS;
        }
        else if( RDKC_PROP_ALGORITHM == PropID ){
                val = (float)va_alg;
                return VA_SUCCESS;
        }
	else if( RDKC_PROP_SENSITIVITY == PropID ) {
		val = (float)sensitivity;
		return VA_SUCCESS;
	}
	else if( RDKC_PROP_UPSCALE_RESOLUTION == PropID ) {
		val = (float)upscale_resolution;
		return VA_SUCCESS;
	}
	return VA_FAILURE;

}

/** @descripion: Initialise VA algorithm
 *  @param: void
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAInit()
{
        switch(va_alg) {
                case VA_ALG_GMM:
	                bgs = new MixtureOfGaussianV2BGS;
        	        //cout << "Using MixtureOfGaussianV2BGS" << endl;
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Using MixtureOfGaussianV2BGS\n", __FUNCTION__, __LINE__);
        	        break;

                case VA_ALG_FD:
                	bgs = new FrameDifferenceBGS;
	                //cout << "Using FrameDifferenceBGS" << endl;
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Using FrameDifferenceBGS\n", __FUNCTION__, __LINE__);
	                break;

                case VA_ALG_PBAS:
        	        bgs = new PixelBasedAdaptiveSegmenter;
                	//cout << "Using PixelBasedAdaptiveSegmenter" << endl;
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Using PixelBasedAdaptiveSegmenter\n", __FUNCTION__, __LINE__);
                	break;

                case VA_ALG_DPWREN:
	                bgs = new DPWrenGABGS;
        	        //cout << "Using DPWrenGABGS" << endl;
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Using DPWrenGABGS\n", __FUNCTION__, __LINE__);
        	        break;

                case VA_ALG_LBASOM:
	                bgs = new LBAdaptiveSOM;
        	        //cout << "Using LBAdaptiveSOM" << endl;
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Using LBAdaptiveSOM\n", __FUNCTION__, __LINE__);
	                break;

                default:
	                bgs = new MixtureOfGaussianV2BGS;
        	        //cout << "Using MixtureOfGaussianV2BGS" << endl;
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): Using MixtureOfGaussianV2BGS\n", __FUNCTION__, __LINE__);
               		break;
        }

	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): xVision runs on %d*%d YUV data\n", __FUNCTION__, __LINE__, DEFAULT_WIDTH, DEFAULT_HEIGHT);
	return VA_SUCCESS;
}

/** @descripion: This function is used to reset certain variables
 *  @param[in] width - width of the frame
 *  @param[in] height - height of the frame
 *  @return void
 */
void VideoAnalytics::RdkCVAReset(int width,int height)
{
        //reset the coordinates values
        first_x=width;
        first_y=height;
        last_x=0;
        last_y=0;
        is_ObjectDetected = false;
        is_MotionDetected = false;
#ifdef _ROI_ENABLED_
	is_MotionInROI = false;
#endif
        numOfObjectsDetected = 0;
        numberOfEventsDetected = 0;
	motionLevelPercentage = 0.0;
	noOfPixelsInMotion = 0.0;
        md_obj_ptr = md_object;
        memset( md_obj_ptr, 0, sizeof(iObject) );

        if (firstFrame == true) {
        	//Initial scaling matrix for creating object detection bounding box
		scaleMat = cv::Mat_<float>::eye(cv::Size(3,3));

		// when requesting lower resolution, the camera hardware
		// preserves the aspect ratio by cropping the left/right
		// margin first (i.e. removing the "pillarbox" bars)
		scaleMat.ptr<float>(0)[0] = (upscale_height / height);
		scaleMat.ptr<float>(1)[1] = (upscale_height / height);
        }
	return;
}

/** @descripion: This function is used to detect objects in current frame
 *  @param[in] img_input - original frame data
 *  @param[in] img_mask - image mask after processing frame data
 *  @return void
 */
void VideoAnalytics::RdkCVADetectObjects(cv::Mat img_input, cv::Mat img_mask)
{
	// filter blobs based on blob tracking, size and aspect ratio
	double varTrack = 0;
	// motion score
	int mScore = 0;
	// Rect object to hold coordinates of object bounding box
	cv::Rect unionBox;
#ifdef _OBJ_DETECTION_
       // Rect object to hold coordinates of object bounding box for detection
       cv::Rect detectionUnionbox;
#endif
	// vector of rects to hold bounding boxes of each individual blob
	std::vector<cv::Rect> bboxs;
	//double Area = 0;

        // Reset Object and Motion Events. These are class variables
        is_MotionDetected = false;
        is_ObjectDetected = false;
#ifdef _ROI_ENABLED_
        // If roiCoords is cleared, complete screen will be the region of interest
        bool hasRoi = roiCoords.size() > 0;
        if(hasRoi) {
                is_MotionInROI = false;
        } else {
                is_MotionInROI =true;
        }
#endif
	is_MotionInDOI = false;

	if(!blobs.empty()) {
		cvb::cvReleaseBlobs(blobs);
	}

	int maxActiveTime = blobTracking.process(img_input, img_mask, img_output, blobs, varTrack, noOfPixelsInMotion);

#ifdef _ROI_ENABLED_
	// update the track history
	const cvb::CvTracks frameTracks = blobTracking.getTracks();
	history.updateTracks(frameTracks);
#endif

    //if((maxActiveTime > 1.0/sensitivity*LOWER_LIMIT_MAXACTIVETIME) && (maxActiveTime < UPPER_LIMIT_MAXACTIVETIME) && (varTrack > (1.0/sensitivity)*(1.0/sensitivity)*(img_input.cols * img_input.rows/352/240.0) * 95)) {
#ifndef _OBJ_DETECTION_
    if ((maxActiveTime > 1.0 / sensitivity * LOWER_LIMIT_MAXACTIVETIME)
        && (maxActiveTime < UPPER_LIMIT_MAXACTIVETIME)
        && (varTrack >
            (1.0 / sensitivity) * (1.0 / sensitivity) * (img_input.cols * img_input.rows / 320.0 / 200.0) * VARIANCE_MULTIPLIER * FOV_SCALE_FACTOR))
	{
#endif

		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): Detecting Objects... Potential object may be present  maxActiveTime[%d] sensitivity[%d] varTrack[%f] img_input.cols[%d] img_input.rows[%d] !!!\n", __FILE__, __LINE__, maxActiveTime, sensitivity, varTrack, img_input.cols, img_input.rows);
		mScore = (int)RdkCVAGetMotionScore(MS_MAX_BLOBAREA);
#ifdef _OBJ_DETECTION_
                RdkCVACreateObjectBBox(img_input.cols, img_input.rows, blobs, unionBox, detectionUnionbox, bboxs);
#else
		RdkCVACreateObjectBBox(img_input.cols, img_input.rows, blobs, unionBox, bboxs);
#endif
		//Load global object bbox array
		uboxCoords[0] = unionBox.x;
		uboxCoords[1] = unionBox.y;
		uboxCoords[2] = unionBox.width;
		uboxCoords[3] = unionBox.height;
#ifdef _OBJ_DETECTION_
                detectionUboxCoords[0] = detectionUnionbox.x;
                detectionUboxCoords[1] = detectionUnionbox.y;
                detectionUboxCoords[2] = detectionUnionbox.width;
                detectionUboxCoords[3] = detectionUnionbox.height;
#endif
		// fill global vector of bounding boxes represented as {x, y, w, h}
		// -- we know that bboxs will be of max size UPPER_LIMIT_BLOB_BBS, so we know we can't
		// -- get index out of bounds since blobBBoxCoords is size (4 * UPPER_LIMIT_BLOB_BBS)
		// clear array (-1 means no bounding box)
               for (size_t i = 0; i < 4 * UPPER_LIMIT_BLOB_BBS; ++i) {
                   blobBBoxCoords[i] = INVALID_BBOX_ORD;
               }
		size_t index = 0;
		for (size_t i = 0; i < bboxs.size(); ++i){
			const cv::Rect &bb = bboxs[i];
			blobBBoxCoords[index + 0] = static_cast<short>(bb.x);
			blobBBoxCoords[index + 1] = static_cast<short>(bb.y);
			blobBBoxCoords[index + 2] = static_cast<short>(bb.width);
			blobBBoxCoords[index + 3] = static_cast<short>(bb.height);
			index += 4;
		}
		// for (size_t i = 0; i < (sizeof(blobBBoxCoords)/sizeof(*blobBBoxCoords)); ++i)
		// 	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): -- Blob BB Coords %d -> [%d] !!!\n", __FILE__, __LINE__, i, blobBBoxCoords[i]);

#ifdef _OBJ_DETECTION_
    if ((maxActiveTime > 1.0 / sensitivity * LOWER_LIMIT_MAXACTIVETIME)
        && (maxActiveTime < UPPER_LIMIT_MAXACTIVETIME)
        && (varTrack >
            (1.0 / sensitivity) * (1.0 / sensitivity) * (img_input.cols * img_input.rows / 320.0 / 200.0) * VARIANCE_MULTIPLIER * FOV_SCALE_FACTOR))
	{
#endif
		for (cvb::CvBlobs::const_iterator it=blobs.begin(); it!=blobs.end(); ++it)
		{
			cvb::CvBlob *blob=(*it).second;
			first_x = blob->minx;
			last_x = blob->maxx;
			first_y = blob->miny;
			last_y = blob->maxy;

			numOfObjectsDetected++;

			if( numOfObjectsDetected <= RDKC_OD_MAX_NUM ) {
				RdkCVAFillDetectedUnknownObjectDetail();
			}
			else {
				break;
			}

			is_ObjectDetected = true;
			is_MotionDetected = true;
		}
		if(true == is_ObjectDetected ) {
			RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d) Detected Object and Motion in the frame\n", __FILE__, __LINE__);
		}

#ifdef _ROI_ENABLED_
		// check whether any of the tracks that could have triggered motion on this frame have
		// ever entered the ROI
		if (hasRoi) {
			// convert the coordinates to a Wykobi polygon and scale it to our process size - do not
			// define the roi in the header due to runtime errors; this is a cheap computation, so
			// doing it here is fine
			polyutils::Polygon roi = polyutils::polyFromPoints(roiCoords);

			// first, collect the track IDs that are visible in this frame
			std::unordered_set<cvb::CvID> frameTrackIds;
			for (auto it = frameTracks.begin(); it != frameTracks.end(); it++) {
				cvb::CvTrack* tt = (*it).second;
				frameTrackIds.insert(tt->id);
			}
			// now, collect the subset of those tracks that could have triggered motion in the
			// present or past
			const auto currentValidTracks = history.getValidTracksFromSubset(frameTrackIds);

			// iterate over all tracks and determine whether any overlapped with the ROI
			// TODO Optimizations:
			// TODO  - Cache which tracks have already been checked
			// TODO  - Store track polygon instead of bounding box
			// TODO  - Don't store zero area tracks in history
			float largestOverlapPercentage = -1;
			for (const auto& tt : currentValidTracks) {
				if (tt.bb.area() <= 0){ continue; } // skip empty tracks
				float overlap = polyutils::intersectOverlapArea(tt.bb, roi); // get intersection area
				// if the overlap area is greater than one we've already seen
				if (overlap > largestOverlapPercentage){
					largestOverlapPercentage = overlap;
					// if the overlap area is greater than the threshold, mark is as within the ROI
					// and break since we don't care about the rest
					if (largestOverlapPercentage > roiOverlapThresh){
						is_MotionInROI = true;
						RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): Motion in ROI Triggered!\n", __FILE__, __LINE__);
						break; // we can stop when we know a track has been within the ROI
					}
				}
                          }
               }
#endif
               if(!DOIBitmap.empty()) {
			// first, collect the track IDs that are visible in this frame
			std::unordered_set<cvb::CvID> frameTrackIds;
			for (auto it = frameTracks.begin(); it != frameTracks.end(); it++) {
				cvb::CvTrack* tt = (*it).second;
				frameTrackIds.insert(tt->id);
			}
			// now, collect the subset of those tracks that could have triggered motion in the
			// present or past
			const auto currentValidTracks = history.getValidTracksFromSubset(frameTrackIds);

			// Check if any of the motion blobs overlaps with DOI
			for (const auto& tt : currentValidTracks) {
				if (tt.bb.area() <= 0){ continue; } // skip empty tracks

#if 0
				int bottomLeftX = tt.bb.x;
				int bottomLeftY = tt.bb.y + tt.bb.height - 1;
				int bottomRightX = tt.bb.x + tt.bb.width - 1;
				int bottomRightY = tt.bb.y;

				int bottomLeft = DOIBitmap.at<uint8_t>(bottomLeftY, bottomLeftX);
				int bottomRight= DOIBitmap.at<uint8_t>(bottomRightY, bottomRightX);

				if(bottomLeft == 255 && bottomRight == 255) {
#else

				cv::Mat blobGrayImage;

				// Create a zero matrix
				cv::Mat blobImage = Mat::zeros(DEFAULT_HEIGHT, DEFAULT_WIDTH, CV_8UC3);
				// Draw a filled rectangle of motion blob in the zero matrix
				cv::rectangle(blobImage, tt.bb, cv::Scalar(255, 255, 255), cv::FILLED);
				cv::cvtColor(blobImage, blobGrayImage, cv::COLOR_BGR2GRAY);
				// Apply threshold to get the matrix as 0 or 255 as member elements, where
				// all elements in motion blob is 255 and others are 0.
		                cv::threshold(blobGrayImage, blobGrayImage, 1, 255, cv::THRESH_BINARY);
				cv::Mat result;
				// Do bitwise and of motion blob and DOI to create intersection matrix
				cv::bitwise_and(DOIBitmap, blobGrayImage, result);
				// Non zero elements in matrix shows intersection.
				// If there is any non-zero elements, it is DOI motion
				// threshold is 30% of total pixels in motion blobs 
				if(cv::countNonZero(result) > (doiOverlapThreshold * tt.bb.area())) {
#endif
					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VIDEOANALYTICS","%s(%d): DOI Motion triggered!\n", __FILE__, __LINE__);
					is_MotionInDOI = true;
					break;
				}
			}
		} else {
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VIDEOANALYTICS","%s(%d): Setting DOI Motion true since no DOI set\n", __FILE__, __LINE__);
			is_MotionInDOI = true;
		}
	}
	//cvb::cvReleaseBlobs(blobs);

	motionLevelPercentage = (float) (100*noOfPixelsInMotion)/frameArea; /* Calculates how many percentage of pixels are in motion irrespective of motion detected or not */

	if( true == is_MotionDetected ) {
		//noOfPixelsInMotion = Area; /* All white pixels are the pixel in motion */

		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d) Motion Level Percentage : %f noOfPixelsInMotion %lf frameArea: %d \n", __FUNCTION__, __LINE__, motionLevelPercentage, noOfPixelsInMotion, frameArea);
		RdkCVAFillDetectedMotionEventDetail();
	}
	return;
}

void VideoAnalytics::RdkCVACreateObjectBBox(
	int motionFrameWidth,
	int motionFrameHeight,
	cvb::CvBlobs &blobs,
	cv::Rect &unionBox,
#ifdef _OBJ_DETECTION_
       cv::Rect &detectionUnionbox,
#endif
	std::vector<cv::Rect> &bboxs
)
{
	bboxs.clear();
#ifdef _ROI_ENABLED_
        bool hasRoi = roiCoords.size() > 0;
        polyutils::Polygon roi = polyutils::polyFromPoints(roiCoords);
#endif
	// vector of blob bounding boxes and corresponding blob areas so we can sort later and take top
	// UPPER_LIMIT_BLOB_BBS bounding boxes
	std::vector<std::pair<cv::Rect, double> > bboxs_and_areas;
	//cv::Rect img_roi(0, 0, img_input.cols, img_input.rows);
	cv::Rect img_roi(0, 0, upscale_width, upscale_height);
	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): Creating Object Box bboxs.size[%d] !!!\n", __FILE__, __LINE__, bboxs.size());

	float blobShift_x = (upscale_width - ((upscale_height / motionFrameHeight) *  motionFrameWidth)) / 2.0f;
	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): blobShift_x=[%f] !!!\n", __FILE__, __LINE__, blobShift_x);

	// loop through blobs and scale them back up to the original image dimensions
	for (cvb::CvBlobs::const_iterator it=blobs.begin(); it!=blobs.end(); ++it)
	{
		cvb::CvBlob *blob = (*it).second;

		cv::Rect blobRect(blob->minx, blob->miny, blob->maxx - blob->minx + 1, blob->maxy - blob->miny + 1);
		bool insideROI = true, insideDOI = true;
#ifdef _ROI_ENABLED_
                if(hasRoi) {
                    float overlap = polyutils::intersectOverlapArea(blobRect, roi);
                    if (overlap > roiOverlapThresh) {
                        insideROI = true;
                    } else {
                        insideROI = false;
		    }
                }
#endif
		if(!DOIBitmap.empty()) {
		    cv::Mat blobGrayImage;

                    // Create a zero matrix
                    cv::Mat blobImage = Mat::zeros(DEFAULT_HEIGHT, DEFAULT_WIDTH, CV_8UC3);
                    // Draw a filled rectangle of motion blob in the zero matrix
                    cv::rectangle(blobImage, blobRect, cv::Scalar(255, 255, 255), cv::FILLED);
                    cv::cvtColor(blobImage, blobGrayImage, cv::COLOR_BGR2GRAY);
                    // Apply threshold to get the matrix as 0 or 255 as member elements, where
                    // all elements in motion blob is 255 and others are 0.
                    cv::threshold(blobGrayImage, blobGrayImage, 1, 255, cv::THRESH_BINARY);
                    cv::Mat result;
                    // Do bitwise and of motion blob and DOI to create intersection matrix
                    cv::bitwise_and(DOIBitmap, blobGrayImage, result);
                    // Non zero elements in matrix shows intersection.
                    // If there is any non-zero elements, it is DOI motion
                    // threshold is 30% of total pixels in motion blobs
                    if(cv::countNonZero(result) > (doiOverlapThreshold * blobRect.area())) {
                        insideDOI = true;
                    } else {
			insideDOI = false;
		    }
		}

                if(!insideROI && !insideDOI) {
		    continue;
                }
		float tldata [] = { (float)blobRect.x, (float)blobRect.y, 1.0f };
		cv::Mat_<float> tlVec(3,1,tldata);
		float brdata [] = { (float)blobRect.x + blobRect.width - 1, (float)blobRect.y + blobRect.height - 1, 1.0f };
		cv::Mat_<float> brVec(3,1,brdata);

		tlVec = scaleMat * tlVec;
		brVec = scaleMat * brVec;

		tlVec.ptr<float>(0)[0] += blobShift_x;
		brVec.ptr<float>(0)[0] += blobShift_x;

		blobRect = cv::Rect_<float>(tlVec.ptr<float>(0)[0], tlVec.ptr<float>(1)[0],  brVec.ptr<float>(0)[0] - tlVec.ptr<float>(0)[0], brVec.ptr<float>(1)[0] - tlVec.ptr<float>(1)[0]);
		bboxs_and_areas.push_back(std::make_pair(blobRect, blob->area));
	}

	//vector<cv::Rect>::const_iterator it = bboxs.begin();
	//while (it != bboxs.end())
	// iterate through blob bounding boxes, upscale each, and create a bounding box containing the
	// union of all
	for(size_t i = 0; i < bboxs_and_areas.size(); i++)
	{
		// pair is a pair of blob bounding box rect and corresponding blob area
		std::pair<cv::Rect, double> &pair = bboxs_and_areas[i];

		cv::Rect bBox;

		// Add increase area around blob
		bBox = upscaleCurrentRect(pair.first, 3.5f);

		// If necessary crop the new upscaled bbox to fit inside img_roi
		bBox = fitInsideBigRect(img_roi, bBox);

		// replace the original bounding box with the upscaled version
		//pair.first = bBox;

		// Union the new blob into the overall union box
		if (i == 0) // on first iteration, the union is just the blob bounding box itself
			unionBox = bBox;
		else
			unionBox |= bBox; // r1|r2 operator is minimum area rectangle containing r1 and r2
#ifdef _OBJ_DETECTION_
                cv::Rect d_bBox;

                // Add increase area around blob
                d_bBox = upscaleCurrentRect(pair.first, deliveryUpscaleFactor);

                // If necessary crop the new upscaled bbox to fit inside img_roi
                d_bBox = fitInsideBigRect(img_roi, d_bBox);

                // Union the new blob into the overall union box
                if (i == 0) // on first iteration, the union is just the blob bounding box itself
                        detectionUnionbox = d_bBox;
                else
                        detectionUnionbox |= d_bBox; // r1|r2 operator is minimum area rectangle containing r1 and r2
#endif
	}

	// if we have more than UPPER_LIMIT_BLOB_BBS blobs, we need to sort them by area and take the
	// top UPPER_LIMIT_BLOB_BBS blobs
	if (bboxs_and_areas.size() > UPPER_LIMIT_BLOB_BBS){
		// sort the bounding boxes by original blob area
		std::sort(bboxs_and_areas.begin(), bboxs_and_areas.end(), RdkCVACompareRectAreaPair);

		// take the top UPPER_LIMIT_BLOB_BBS and place them in bboxs
		for (size_t i = 0; i < UPPER_LIMIT_BLOB_BBS; ++i){
			bboxs.push_back(bboxs_and_areas[i].first);
		}
	}
	else{
		// place each blob bounding box in bboxs
		for (size_t i = 0; i < bboxs_and_areas.size(); ++i){
			bboxs.push_back(bboxs_and_areas[i].first);
		}
	}

	// RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): Union box OpenCV XYWH -> [%d, %d, %d, %d] !!!\n", __FILE__, __LINE__, cv_unionBox.x, cv_unionBox.y, cv_unionBox.width, cv_unionBox.height);
	// RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): Number of blob bounding boxes [%d] !!!\n", __FILE__, __LINE__, bboxs.size());
	// for (size_t i = 0; i < bboxs.size(); ++i)
	// 	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): -- Blob BB %d XYWH -> [%d, %d, %d, %d] !!!\n", __FILE__, __LINE__, i, bboxs[i].x, bboxs[i].y, bboxs[i].width, bboxs[i].height);
	return;
}

#ifdef _OBJ_DETECTION_
/** @description: Get object detction bounding box coordinates
 *  #param[in] bbox_coords : Co-ord of ROI
 */
int VideoAnalytics::RdkCVAGetDeliveryObjectBBox(short *bbox_coords)
{
        //bbox_coords = detectionUboxCoords;
        bbox_coords[0] = static_cast<short>(detectionUboxCoords[0]);
        bbox_coords[1] = static_cast<short>(detectionUboxCoords[1]);
        bbox_coords[2] = static_cast<short>(detectionUboxCoords[2]);
        bbox_coords[3] = static_cast<short>(detectionUboxCoords[3]);

        return VA_SUCCESS;
}

int VideoAnalytics::RdkCVASetDeliveryUpscaleFactor(float scaleFactor)
{
	deliveryUpscaleFactor = scaleFactor;
        return VA_SUCCESS;
}
#endif

/** @description: Set DOI overlap threshold
 *  #param[in] threshold: overlap threshold
 */
int VideoAnalytics::RdkCVASetDOIOverlapThreshold(float threshold)
{
	doiOverlapThreshold = threshold;
        return VA_SUCCESS;
}

/** @description: Get object detction bounding box coordinates
 *  #param[in] bbox_coords : Co-ord of ROI
 */
int VideoAnalytics::RdkCVAGetObjectBBoxCoords(short *bbox_coords)
{
	//bbox_coords = uboxCoords;
	bbox_coords[0] = static_cast<short>(uboxCoords[0]);
	bbox_coords[1] = static_cast<short>(uboxCoords[1]);
	bbox_coords[2] = static_cast<short>(uboxCoords[2]);
	bbox_coords[3] = static_cast<short>(uboxCoords[3]);

	return VA_SUCCESS;
}

/** @description: Get bounding box coordinates of each individual blob detected in object detection
 *  #param[in] bboxs : array of coordinates of ROI
 */
int VideoAnalytics::RdkCVAGetBlobsBBoxCoords(short *bboxs)
{
	// fill the array with coordinates
	for (size_t i = 0; i < 4 * UPPER_LIMIT_BLOB_BBS; ++i){
		bboxs[i] = blobBBoxCoords[i];
	}

	return VA_SUCCESS;
}

/** @description: Get the XCV version
 *  #param[in] versionStr : loads and returns the version
 */
int VideoAnalytics::RdkCVAGetXCVVersion(char *version_str, int& versionlength)
{
	strncpy(version_str, XCV_VERSION(XCV_VERSION_MAJOR, XCV_VERSION_MINOR, XCV_VERSION_REVISION),versionlength);
	version_str[versionlength] = '\0';
	return VA_SUCCESS;
}

/** @description: Get total blob area for current frame.
 *  @param[in]  blob_threshold : filter to select the blob areas for calculating
                                 total blob area for current frame.
 *  @param[out] currentBlobArea : output parameter for total blob area for
                                  current frame.
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCGetCurrentBlobArea( double blobThreshold , double *currentBlobArea)
{
	*currentBlobArea = noOfPixelsInMotion;   //blobTracking calculates and returns total blob area
											 //per motion frame.

/*
	double blobsArea = 0;
	cvb::CvTracks tracks = blobTracking.getTracks();

	for (cvb::CvTracks::const_iterator it = tracks.begin(); it != tracks.end(); it++) {
		cvb::CvTrack* track = it->second;
		cvb::CvLabel label = track->label;
		cvb::CvBlob* blob = blobs[label];
		if (blob->area >= blobThreshold && track->lifetime > 1) {
			blobsArea += blob->area;
		}
	}
	*currentBlobArea = blobsArea;
*/

	return VA_SUCCESS;
}

/** @descripion: This function is used to detect objects in current frame
 *  @param[in] data - frame pointer
 *  @param[in] size - frame size
 *  @param[in] height - frame height
 *  @param[in] width - frame width
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAProcessFrame( unsigned char *data, int size, int height, int width )
{

	img_input = cv::Mat( height, width, CV_8UC1, data );
#if defined(XCAM2) || defined(XCAM3)
        //Ignoring the i/p frame buffer resolution to resize it to 320x180(16:9 aspect ratio)
        width = DEFAULT_WIDTH;
        height = DEFAULT_HEIGHT;
        size = width * height;
        //------------------------------------------------
#endif

	frameArea = size;
	RdkCVAReset(width,height);
	//cv::setNumThreads(8);

	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): Processing Frame of size[%d] width[%d] heoght[%d] !!!\n", __FILE__, __LINE__, size, width, height);

#if defined(XCAM2) || defined(XCAM3)
        //Resizing frame buffer to 320*180(16:9) resolution
        cv::resize(img_input, img_input, cv::Size(DEFAULT_WIDTH, DEFAULT_HEIGHT));
        //------------------------------------------------
#endif

	GaussianBlur(img_input, img_input, Size(3,3), 0,0);

	/* Set the min and max area only once */
	if(firstFrame) {

#if defined(DBC)
		blobTracking.setMinArea((width*height*FOV_SCALE_FACTOR_SQR)/1500);
		blobTracking.setMaxArea((width*height*FOV_SCALE_FACTOR_SQR));
#elif defined(XCAM2) || defined(XCAM3)
		blobTracking.setMinArea((width*height*FOV_SCALE_FACTOR_SQR)/1125);
		blobTracking.setMaxArea((width*height*FOV_SCALE_FACTOR_SQR)/15);
#else
		blobTracking.setMinArea((width*height*FOV_SCALE_FACTOR_SQR)/1500);
		blobTracking.setMaxArea((width*height*FOV_SCALE_FACTOR_SQR)/20);
#endif
		blobTracking.setThresholdDistance((double)FOV_SCALE_FACTOR);
	}

	if( NULL != bgs ) {
	        bgs->process(img_input, img_mask, img_bkgmodel); // by default, it shows automatically the foreground mask image
	}
	else {
		return VA_FAILURE;
	}

        blobTracking.setShowOutput(false);

        if (!img_mask.empty() && !firstFrame)
        {
		RdkCVADetectObjects(img_input,img_mask);
	    	//noOfPixelsInMotion = countNonZero(img_mask); /* All white pixels are the pixel in motion */
		//motionLevelPercentage = (float) (100*noOfPixelsInMotion)/(width*height); /* Calculates how many percentage of pixels are in motion */
      	}
        firstFrame = false;

        if( true == is_ObjectDetected ) {
                return RDKC_OBJECT_DETECTED;
        } else {
                return RDKC_OBJECT_NOT_DETECTED;
        }
}

/** @description: Get Motion Level for frame.
 *  @param: void
 *  @return: float
 */
float VideoAnalytics::RdkCVAGetMotionLevel()
{
	float range_sub_unit = 0.0;
	float motionLevel = 0.0;
	/*if( false == is_ObjectDetected ) {
		motionLevel = 0.0;
		return round(motionLevel);
	}*/

	/* If the 25% (or more) pixels under motion, then motion level is 100 */
	if( motionLevelPercentage >= MAX_MOTION_LEVEL_PERCENTAGE ) {
		motionLevel = 100;
	}
	else {
		/* Calculate the motion level as per below formula*/
		/* <motion level> = 100 * sqrt(<pixels in motion> / <frame area> * 2) */
		motionLevel = 100 * sqrt( noOfPixelsInMotion / frameArea * 2);
	}
	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): Motion Level %f Round Motion Level %f \n", __FUNCTION__, __LINE__, motionLevel, round(motionLevel));
	return round(motionLevel);

}

/** @description: Get Raw Motion Level for frame.
 *  @param: void
 *  @return: float
 */
float VideoAnalytics::RdkCVAGetRawMotionLevel()
{
	if( false == is_ObjectDetected ) {
		return 0;
	}
	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.VIDEOANALYTICS","%s(%d): Raw Motion Level %f \n", __FUNCTION__, __LINE__, motionLevelPercentage);
	return motionLevelPercentage;
}

/** @description: Get Object count for frame.
 *  @param: void
 *  @return: total number of objects detected
 */
int VideoAnalytics::RdkCVAGetObjectCount()
{
	return numOfObjectsDetected;
}

/** @description: This function is used to get detected object detaisl
 *  @param[out] refObj: Structure object for which memory should be allocated by the application who is calling it.
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAGetObject(iObject *refObj)
{
	if( RDKC_OD_MAX_NUM < numOfObjectsDetected ) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): %d total objects detected. %d objects ignored!", __FUNCTION__, __LINE__,numOfObjectsDetected,(numOfObjectsDetected - RDKC_OD_MAX_NUM));
		numOfObjectsDetected = RDKC_OD_MAX_NUM;
	}
        md_obj_ptr = md_object;
	iObject *refObj_ptr = refObj;
	if( (NULL != md_obj_ptr) && (NULL != refObj_ptr) ) {
		if( (true == is_ObjectDetected) && (numOfObjectsDetected > 0) ) {
			for( int count = 0; count < numOfObjectsDetected; count++) {
			        memcpy( (iObject*) refObj_ptr, md_obj_ptr, sizeof(*md_obj_ptr) );
				md_obj_ptr++;
				refObj_ptr++;
			}
		}
	}
	else {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): md_obj_ptr or refObj pointer is not valid!",__FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	md_obj_ptr = md_object;
	return VA_SUCCESS;
}

/**
 * @brief This function is used to fill details of the detected object.
 * @param Void
 * @return VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAFillDetectedUnknownObjectDetail()
{
	if( NULL != md_obj_ptr ) {
	        md_obj_ptr->m_e_class = eOC_Unknown;
        	md_obj_ptr->m_e_objCategory = eUncategorized;
	        md_obj_ptr->m_i_ID = DEFAULT_M_I_ID;
        	md_obj_ptr->m_e_state = eObject_Detected;
	        md_obj_ptr->m_f_confidence = DEFAULT_M_F_CONFIDENCE;
        	md_obj_ptr->m_u_bBox.m_i_lCol = first_x * scaleFactor;
	        md_obj_ptr->m_u_bBox.m_i_tRow = first_y * scaleFactor;
        	md_obj_ptr->m_u_bBox.m_i_rCol = last_x * scaleFactor;
	        md_obj_ptr->m_u_bBox.m_i_bRow = last_y * scaleFactor;
        	md_obj_ptr++;
	}
	else {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): md_obj_ptr pointer is not valid!",__FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	return VA_SUCCESS;
}

/** @description: Get Object count for frame.
 *  @param: void
 *  @return: total number of events detected
 */
int VideoAnalytics::RdkCVAGetEventCount()
{
	return numberOfEventsDetected;
}

/** @description: This function is used to get event details
 *  @param[out] refObj: Structure Event for which memory has to
 *                 allocate by the application who is calling this function.
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAGetEvent(iEvent *refEvent)
{
	if( (NULL != md_evt) && (NULL != refEvent) ) {
		if( true == is_MotionDetected ) {
			memcpy( (iEvent*) refEvent, md_evt, sizeof(iEvent) );
		}
	}
	else {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): md_evt or refEvent pointer is not valid!",__FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	return VA_SUCCESS;
}

/**
 * @brief This function is used to fill details of the detected motion.
 * @param void
 * @return VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAFillDetectedMotionEventDetail()
{
	numberOfEventsDetected++;

	time_t     now = time(0);
	struct tm  tstruct;
	tstruct = *localtime(&now);

	if( NULL != md_evt ) {
		md_evt -> m_e_type = eMotionDetected;
		md_evt -> m_dt_time.m_i_year = (unsigned int) tstruct.tm_year+1900;
		md_evt -> m_dt_time.m_i_month = (unsigned int) tstruct.tm_mon+1;
		md_evt -> m_dt_time.m_i_day = (unsigned int) tstruct.tm_mday;
		md_evt -> m_dt_time.m_i_hour = (unsigned int) tstruct.tm_hour;
		md_evt -> m_dt_time.m_i_minute = (unsigned int) tstruct.tm_min;
		md_evt -> m_dt_time.m_i_second = (unsigned int) tstruct.tm_sec;
	}
	else {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): md_evt pointer is not valid!",__FUNCTION__, __LINE__);
		return VA_FAILURE;
	}
	return VA_SUCCESS;
}

/** @description: Reset all video-analytics components.
 *  @param: void
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAResetAlgorithm()
{
        if( NULL != bgs ) {
                delete bgs;
                bgs = NULL;
        }
	else {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): bgs pointer is not valid!",__FUNCTION__, __LINE__);
		return VA_FAILURE;
	}


        firstFrame = true;
	RdkCVAInit();
	return VA_SUCCESS;

}
/** @description: Release all the occupied memory.
 *  @param: void
 *  @return: void
 */
void VideoAnalytics::RdkCVARelease()
{
	if( NULL != md_object ){
		free( md_object );
		md_object = NULL;
	}
	if( NULL != md_evt ){
		delete md_evt;
		md_evt = NULL;
	}
	if( NULL != bgs ){
		delete bgs;
		bgs = NULL;
	}
	return;
}

/** @description: Calculate and retrieve motion score for current frame.
 *  @param: void
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
float VideoAnalytics::RdkCVAGetMotionScore(int mode)
{
    switch(mode)
    {
            case MS_MAX_BLOBAREA:
            	motionScore = 100 * sqrt( noOfPixelsInMotion / frameArea * 2);
    	        break;

            default:
            	motionScore = 100 * sqrt( noOfPixelsInMotion / frameArea * 2);
        	break;
    }

    return motionScore;
}

/** @descripion: Helper function to assist in sorting blobs by area by comparing pairs of blob bbs
 * 				 and their corresponding blob areas.
 *  @param[in] a - the first pair to compare
 *  @param[in] b - the second pair to compare
 *  @return: whether the second[double] component of a is greater than b
 */
bool VideoAnalytics::RdkCVACompareRectAreaPair(const std::pair<cv::Rect, double> &a, const std::pair<cv::Rect, double> &b)
{
	return a.second > b.second;
}

#ifdef _ROI_ENABLED_
/** @descripion: Set ROI
 *  @param[in] coords - coordinates list
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVASetROI(std::vector<float> coords)
{
	for( int j = 0; j < coords.size(); j++) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): coords[%d] : %f\n",__FUNCTION__, __LINE__, j, coords[j]);
	}

	if( (!coords.empty() && (coords.size() == 1) && (coords[0] == 0)) || coords.empty() )
	{
		RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VIDEOANALYTICS","%s(%d): ROI disabled. Clearing ROI\n",__FUNCTION__, __LINE__);
		roiCoords.clear();
		return VA_SUCCESS;
	}

	//Check if the number of coordinate entries is valid
	if( coords.size() % 2 != 0 ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): SetROI failed: Invalid coordinates\n",__FUNCTION__, __LINE__);
		return VA_FAILURE;
	}

	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VIDEOANALYTICS","%s(%d): Clearing previously set ROI\n",__FUNCTION__, __LINE__);
	roiCoords.clear();

	for( int i = 0; i < coords.size()/2; i++) {
		//Check if the x, y coordinates lies inside upscale resolution
		if(coords[i*2] < 0 || coords[i*2] > 1 || coords[(i*2)+1] < 0 || coords[(i*2)+1] > 1) {
			RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): SetROI failed: Invalid coordinates, clearing ROI\n",__FUNCTION__, __LINE__);
			roiCoords.clear();
			return VA_FAILURE;
		}

		int x = coords[i*2] * DEFAULT_WIDTH;
		int y = coords[(i*2) + 1] * DEFAULT_HEIGHT;
		roiCoords.push_back(cv::Point(x, y));
	}
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VIDEOANALYTICS", "%s(%d): SetROI SUCCESS\n",__FUNCTION__, __LINE__);
	return VA_SUCCESS;
}

/** @descripion: Clear ROI
 *  @param[in]:
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVAClearROI()
{
        if( roiCoords.empty() ) {
		RDK_LOG( RDK_LOG_WARN,"LOG.RDK.VIDEOANALYTICS","%s(%d): ROI is not set\n",__FUNCTION__, __LINE__);
	}
	roiCoords.clear();
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VIDEOANALYTICS", "%s(%d): ClearROI SUCCESS\n",__FUNCTION__, __LINE__);
	return VA_SUCCESS;
}
/** @descripion: Set ROI Overlap Threshold
 *  @param[in] thresh - threshold as a percentage in the range [0, 1]
 *  @return: VA_SUCCESS on success, VA_FAILURE on failure
 */
int VideoAnalytics::RdkCVASetROIOverlapThresh(float thresh)
{
	// make sure the threshold is between 0 and 1, inclusive
	if( thresh < 0 || thresh > 1 ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): SetROIOverlapThresh failed: Invalid threshold, not between 0 and 1 \n",__FUNCTION__, __LINE__);
		return VA_FAILURE;
	}

	roiOverlapThresh = thresh;
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VIDEOANALYTICS", "%s(%d): SetROIOverlapThresh SUCCESS\n",__FUNCTION__, __LINE__);
	return VA_SUCCESS;
}

/** @descripion: Check if track is valid
 *  @param[in]:
 *  @return: true if track is valid, else false
 */
bool isTrackValid(const cvb::CvTrack& track, float minActiveTimeThresh, float maxActiveTimeThresh, float varianceThresh)
{

	float var = sqrt(track.centroid_var.x*track.centroid_var.x + track.centroid_var.y*track.centroid_var.y)/track.active;
	if ((track.active > minActiveTimeThresh) && (track.active < maxActiveTimeThresh) && (var > varianceThresh)) {
		return true;
	}
	return false;
}

/** @descripion: Check if motion is inside ROI
 *  @param[in]:
 *  @return: true if motion is inside ROI, else false
 */
bool VideoAnalytics::RdkCVAIsMotionInsideROI()
{
	return is_MotionInROI;
}
#endif

/** @descripion: Check if motion is inside ROI
 *  @param[in]:
 *  @return: true if motion is inside ROI, else false
 */
bool VideoAnalytics::RdkCVAapplyDOIthreshold(bool enable, const char *doi_path, int doi_threshold)
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VIDEOANALYTICS", "%s(%d): DOI bitmap: %s\n",__FUNCTION__, __LINE__, doi_path);
	
	if (enable) {
		if (0 != access(doi_path, F_OK)) {
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VIDEOANALYTICS", "%s(%d): Error opening DOI bitmap\n",__FUNCTION__, __LINE__);
			return false;
		}
		DOIBitmap = cv::imread(doi_path, cv::IMREAD_GRAYSCALE);
		cv::threshold(DOIBitmap, DOIBitmap, doi_threshold, 255, cv::THRESH_BINARY);
		cv::resize(DOIBitmap, DOIBitmap, cv::Size(DEFAULT_WIDTH, DEFAULT_HEIGHT));
		cv::imwrite(DOI_BITMAP_BINARY_PATH, DOIBitmap);
		if((doi_threshold == 0) || (doi_threshold == 255)) {
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VIDEOANALYTICS", "%s(%d): DOI threshold is %d, hence, setting doi_motion to true by default\n",__FUNCTION__, __LINE__, doi_threshold);
			DOIBitmap.release();
		}
	} else {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VIDEOANALYTICS", "%s(%d): Disabling DOI bitmap\n",__FUNCTION__, __LINE__);
		DOIBitmap.release();
	}
	return true;
}

/** @descripion: Check if motion is inside DOI
 *  @param[in]:
 *  @return: true if motion is inside DOI, else false
 */
bool VideoAnalytics::RdkCVAIsMotionInsideDOI()
{
	return is_MotionInDOI;
}
