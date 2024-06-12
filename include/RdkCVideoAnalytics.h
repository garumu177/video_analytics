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

#ifndef __VIDEOANALYTICS_H_
#define __VIDEOANALYTICS_H_

/*************************       INCLUDES         *************************/
#include <unistd.h>
/* common include */
#include "RdkCVACommon.h"
/* xcv includes */
#include <components/tracking/BlobTracking.h>
#include "xcv_version.h"
#include "objValidation.h"
#include "bgslibrary.h"
#include "rectutils.h"
/* RdkC VA Manager include */
#include "RdkCVAManager.h"

#ifdef _ROI_ENABLED_
#include <components/tracking/track_history.h>
#include "polyutils.h"
#endif

#define DOI_BITMAP_BINARY_PATH "/opt/usr_config/doi_bitmap-binary.jpg"
/* Maximum 6 levels of motion is supported */
#define MAX_LEVEL_OF_MOTION 7

/* Lower and upper limit for MaxActiveTime */
#define LOWER_LIMIT_MAXACTIVETIME 9
#define UPPER_LIMIT_MAXACTIVETIME 1000

/* Upper limit for number of blob bounding boxes */
#define UPPER_LIMIT_BLOB_BBS 5
#define INVALID_BBOX_ORD (-1)

/* Below structure is used to map levels of motion to
   correcponding percentage of detected motion in the frame */
struct motion_level_map_s{
	float range_motion_level;
	float percentage_motion;
};

class VideoAnalytics
{
public:

	/* Constructor */
	VideoAnalytics();
	/* Destructor */
	~VideoAnalytics();
	/* XCV Version */
	int RdkCVAGetXCVVersion(char *version_str, int& versionlength);
	/* Initialise VA */
	int RdkCVAInit();
	/* Set VA Algorithm */
	static int RdkCVASetAlgorithm(int alg);
	/* Get object count */
	int RdkCVAGetObjectCount();
	/* Get object data */
	int RdkCVAGetObject(iObject *refObj);
	/* Get event count */
	int RdkCVAGetEventCount();
	/* Get event data */
	int RdkCVAGetEvent(iEvent *refEvent);
	/* Process frame */
	int RdkCVAProcessFrame( unsigned char *data, int size, int height, int width );
	/* Set Property */
	int RdkCVASetProperty(int PropID, float& val);
	/* Get property */
	int RdkCVAGetProperty(int PropID, float& val);
	/* Reset VA algorithm */
	int RdkCVAResetAlgorithm();
	/* Get Currect Blob area */
	int RdkCGetCurrentBlobArea(double blob_threshold, double *currentBlobArea);
	/* Release memory */
	void RdkCVARelease();
        /* Get motion score technique */
        float RdkCVAGetMotionScore(int mode);
#ifdef _OBJ_DETECTION_
        /* Get object bounding box coordinates */
        int RdkCVAGetDeliveryObjectBBox(short *ubox_coords);

        int RdkCVASetDeliveryUpscaleFactor(float scaleFactor);
#endif
        /* Get object bounding box coordinates */
	int RdkCVAGetObjectBBoxCoords(short *ubox_coords);
	/* Get bounding box coordinates of individual blobs */
	int RdkCVAGetBlobsBBoxCoords(short *bboxs);
#ifdef _ROI_ENABLED_
	/* set ROI Coordinatres */
	int RdkCVASetROI(std::vector<float> coords);
	/* Clear ROI Coordinatres */
	int RdkCVAClearROI();
	/* Set ROI Overlap Threshold */
	int RdkCVASetROIOverlapThresh(float thresh);
	/* Check if motion is inside ROI */
	bool RdkCVAIsMotionInsideROI();
#endif
        /* Apply DOI */
        bool RdkCVAapplyDOIthreshold(bool enable, const char *doi_path, int doi_threshold);
	/* Check if motion is inside DOI */
	bool RdkCVAIsMotionInsideDOI();
	/* Set DOI overlap threshold */
	int RdkCVASetDOIOverlapThreshold(float threshold);

private:
					/* Object coordinates */
					/* fx:first_x,fy_first_y,lx:last_x,ly:last_y */
        int first_x;			/*  (fx,ly) ______(lx,ly)	*/
        int first_y;			/*         |      |		*/
        int last_x;			/*         |      |		*/
        int last_y;			/*  (fx,fy)|______|(lx,fY)	*/
        bool firstFrame;		/* check if first frame */
        bool is_ObjectDetected;		/* check for object detection */
        bool is_MotionDetected;		/* check for motion detection */
	int numOfObjectsDetected;	/* total number of detected objects */
	int numberOfEventsDetected;	/* total number of detected events */
        IBGS *bgs;			/* Background Subtraction Algorithm */
        BlobTracking blobTracking;	/* Blob tracking mechanism */
	iObject *md_object;		/* Object structure for motion detected object details */
        iObject *md_obj_ptr;		/* copy of md_object pointer */
	iEvent *md_evt;		/* Event structure for motion detected object details */
	int scaleFactor;		/* Scale factor. 1 for YUV, 4 for ME1 frame typr */
	float motionLevelPercentage;	/* Percentage of motion in one frame */
	float motionDetection_Enable;	/* Motion Detection is enabled by default */
	float objectDetection_Enable;	/* Object Detection is enabled by default */
	float humanDetection_Enable;	/* Human Detection is disabled by default */
	float tamperDetection_Enable;	/* Tamper Detection is disabled by default */
	static int va_alg;		/* Video-Analytics algorith */
	int sensitivity;		/* sensitivity value according to Day/Night */
	eRdkCUpScaleResolution_t upscale_resolution; /* Upscaling resolution */
	float upscale_width;            /* Upscaling resolution width */
	float upscale_height;           /* Upscaling resolution height */
	double noOfPixelsInMotion;
	float motionScore;
	int frameArea;
	cvb::CvBlobs blobs;
        cv::Mat_<float> scaleMat;       /* Scaling matrix for object bounding box */
#ifdef _OBJ_DETECTION_
       short detectionUboxCoords[4];
       float deliveryUpscaleFactor;
#endif
	float doiOverlapThreshold;
	short uboxCoords[4];
	short blobBBoxCoords[4 * UPPER_LIMIT_BLOB_BBS]; /* Vector of blob bounding boxes {x, y, w, h} */
#ifdef _ROI_ENABLED_
	bool is_MotionInROI;		/* check for motion in ROI */
	std::vector<cv::Point> roiCoords;
	TrackHistory history;   /* The object that stores historical tracks to compare to the ROI */
	float roiOverlapThresh; /* The overlap percentage necessary to trigger motion within an ROI (>) */
#endif
	bool is_MotionInDOI;		/* check for motion in DOI */

#ifdef _OBJ_DETECTION_
        /* Creating bounding box from motion blobs  */
        void RdkCVACreateObjectBBox(int motionFrameWidth, int motionFrameHeight, cvb::CvBlobs &blobs, cv::Rect &unionBox, cv::Rect &detectionUnionbox, std::vector<cv::Rect> &bboxs);
#else
	/* Creating bounding box from motion blobs  */
	void RdkCVACreateObjectBBox(int motionFrameWidth, int motionFrameHeight, cvb::CvBlobs &blobs, cv::Rect &unionBox, std::vector<cv::Rect> &bboxs);
#endif
	/* Map motion level value to motion level percentage */
	void RdkCVAMapMotionLevels();
	/* Fill object detected details into iObject structure */
	int RdkCVAFillDetectedUnknownObjectDetail();
	/* Fill detected event details int iEvent structure */
	int RdkCVAFillDetectedMotionEventDetail();
	/* xcv - Opencv calls to detect objects */
	void RdkCVADetectObjects(cv::Mat img_input, cv::Mat img_mask);
	/* Get motion level */
	float RdkCVAGetMotionLevel();
	/* Get Raw motion level */
	float RdkCVAGetRawMotionLevel();
	/* Reset */
	void RdkCVAReset(int width, int height);
	/* Comparison function used to assist in sorting blobs by area. */
	static bool RdkCVACompareRectAreaPair(const std::pair<cv::Rect, double> &a, const std::pair<cv::Rect, double> &b);
	cv::Mat img_input;
	cv::Mat img_mask;
	cv::Mat img_bkgmodel;
  	cv::Mat img_output;
	cv::Mat DOIBitmap;
};

#ifdef _ROI_ENABLED_
/* Determine whether a track is valid - whether it could have triggered motion - based on
   active motion time and variance thresholds. */
bool isTrackValid(const cvb::CvTrack& track, float minActiveTimeThresh, float maxActiveTimeThresh, float varianceThresh);
#endif

#endif /*#ifndef __VIDEOANALYTICS_H_ */
