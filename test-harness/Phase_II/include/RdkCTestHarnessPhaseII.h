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

#ifndef RDKC_TEST_HARNESS_H
#define RDKC_TEST_HARNESS_H
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <error.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <cstring>
#include <queue>
#include <mutex>

#include "RdkCVACommon.h"

/*    Test-Harness manager   */
#include "RdkCTHManager.h"

/*    Opencv Includes     */
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/core/core.hpp>

/*    Rdklogger Include   */
#include "rdk_debug.h"

/*   configMgr Include	*/
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
#include "config_api.h"
#ifdef __cplusplus
}
#endif //__cplusplus

#define RDKC_SUCCESS             0      /* Success */
#define RDKC_FAILURE            -1      /* Failure */
#define RDKC_FILE_START		 2	/* New file starts */
#define RDKC_FILE_END		 3	/* File End */

using namespace std;
using namespace cv;

#ifdef TEST_HARNESS_SOCKET

#include "socket_client.h"
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/* Below macros are corresponding to configuration file */
#define HOSTNAME "hostname"						/* ip address of the server */
#define PORT "port" 							/* port to connect for socket */
#define TEST_HARNESS_ON_FILE_ENABLED "Test_Harness_on_File_Enabled"	/* frame feed is from camera or file */
#define RESET_ALG_ON_FIRST_FRAME "Reset_Alg_on_First_Frame"		/* reset alg on first frame */
#define TEST_HARNESS_WITH_ROI "Test_Harness_with_ROI"                   /* Testharness with ROI */
#define TEST_HARNESS_WITH_DOI "Test_Harness_with_DOI"                   /* Testharness with DOI */
#define NW_INTERFACE "nw_interface"
#define LOGGER_PATH "/etc/debug.ini"					/* rdklogger path */
#define MAXSIZE 100
#define MACSIZE 17
#define MINSIZE 10

typedef struct frameData_s
{
    unsigned char *y_data;              /* luma plane */
    int y_size;                         /* luma plane size */
    int y_height;                       /* luma plane height */
    int y_width;                        /* luma plane width */
    unsigned char *uv_data;             /* chroma plane */
    int uv_size;                        /* chroma plane size */
    int uv_height;                      /* chroma plane height */
    int uv_width;                       /* chroma plane width */
}frameData_t;


void* ThraedFunction(void* args);

class TestHarness
{
public:

	char enableTestHarnessOnFileFeed[MINSIZE+1];			/* true for file feed, false for camera feed */
        char enableTestHarnessWithROI[MINSIZE+1];                         /* true if ROI is to be read from THServer, else false */
	char resetAlgOnFirstFrame[MINSIZE+1];				/* true if multiple video files are to be assumed continous */
        char enableTestHarnessWithDOI[MINSIZE+1];                         /* true if DOI is to be read from THServer, else false */

        TestHarness();							/* Constructor */
        ~TestHarness();							/* Destructor */
	int RdkCTHInit();
	int RdkCTHCheckConnStatus(const char *ifname);
	int RdkCTHPassFrame(th_iImage *plane0, th_iImage *plane1);	/* get the frame form camera */
	int RdkCTHGetFrame(th_iImage *plane0, th_iImage *plane1, int &fps);	/* get the frame from file */
	int RdkCTHResetPayload();					/* reset the payload to be passed over socket */
	int RdkCTHCreatePayload(int objCount,int evtCount,th_iObject *th_objects,th_iEvent *th_events,bool th_is_motion_inside_ROI, bool th_is_motion_inside_DOI); /* create the payload to be passed over socket */
	int RdkCTHUploadPayload();					/* uopload the payload over socket */
	int RdkCTHProcessFrame();					/* create harnessed data */
	int RdkCTHRelease();						/* releases all the occupied memory */
	int RdkCTHSetVAEngine(const char *VAEngine);			/* set Video-Analytics engine (i.e "IV" or "RDKCVA") */
	int RdkCTHSetImageResolution(int width, int height);		/* set Image resolution */
	bool RdkCTHIsROICoordinatesChanged();
	std::string RdkCTHGetROICoordinates();
	bool RdkCTHIsDOIChanged();
	bool RdkCTHGetDOIConfig(int &threshold);
	int RdkCTHWriteFrame(std::string filename);
	int RdkCTHAddDeliveryResult(th_deliveryResult result);
	int RdkCTHAddSTNUploadStatus(th_smtnUploadStatus stnStatus);

	int RdkCTHGetClipSize(int32_t *clip_size);			/* Gets the clip segmenting size in seconds */

private:
	Mat fileFrameBGR,fileFrameYUV,planeUV;				/* Mats to store color images */
	vector<Mat> yuvChannels, yuvPlanes;				/* vectors to separate yuv channels from RGB */
	cv::Mat RGBMat;							/* Mat contains RGB frame */
	bool yuvDataMemoryAllocationDone;				/* check for yuv data memory allocation */
	frameData_t frame;						/* structure contains y and uv data */
	ProcessedData_s *meta_data;					/* structure contains meta-data for capyured frame */
	TestHarnessInfo_s TH_info;					/* structure contains harnessed info */
	TestHarnessInfo_s TH_info_recv_buf;				/* structure contains harnessed info */
	SocketClient* client_ptr;					/* pointer to socket client */
	char VAEngineID[MINSIZE+1];					/* to store Video-Analytics engine (i.e "IV" or "RDKCVA")*/
	char mac_id[MACSIZE+1];						/* to store mac id of the device */
	unsigned char* yuvData;						/* pointer to contain yuv frame */
	char* configParam;						/* pointer to store data from configuration file */
	int objectCounter;						/* to store number of objects detected */
	int eventCounter;						/* to store number of events detected */
	int imageWidth;							/* image width */
	int imageHeight;						/* image height */
        int32_t clipSize;
	bool fileEnd;
	int th_frame_width;
	int th_frame_height;
	std::queue <TestHarnessInfo_s> THInfoQue;
	std::queue <th_deliveryResult> DeliveryInfoQue;
	std::queue <th_smtnUploadStatus> StnUploadStatusQue;
	pthread_t THUploadThread;
	std::mutex QueueMutex;
	std::mutex DQueMutex;
	bool isROICoordsChanged;
	std::string roiCoords;
	bool isDOIChanged;
	bool doi_enable;
	int doiThreshold;
	int RdkCTHGetMACId();						/* get the mac id of the device */
        int RdkCTHSetObjectsCount(int objCount);			/* Set objects count */
        int RdkCTHSetEventsCount(int evtCount);				/* Set event count */
        int RdkCTHSetObject(th_iObject *object);			/* Set objects and event structure */
        int RdkCTHSetEvent(th_iEvent *event);				/* Set objects and event structure */
	void RdkCTHRescaleBlob(Object_s *object);
};
#ifdef __cplusplus
}
#endif //__cplusplus

#endif //TEST_HARNESS_SOCKET

#endif //RDKC_TEST_HARNESS_H

