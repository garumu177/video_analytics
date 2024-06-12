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

#include "RdkCTestHarnessPhaseII.h"
#ifdef TEST_HARNESS_SOCKET

/* Constructor */
TestHarness::TestHarness() : yuvDataMemoryAllocationDone(false)
                           , meta_data(NULL)
                           , isROICoordsChanged(false)
                           , client_ptr(NULL)
                           , yuvData(NULL)
                           , configParam(NULL)
                           , objectCounter(0)
                           , eventCounter(0)
                           , imageWidth(0)
                           , imageHeight(0)
                           , clipSize(0)
                           , fileEnd(false)
{
        memset(&frame, 0, sizeof(frame));
        memset(&(TH_info.pData), 0, sizeof(ProcessedData_s));
        memset(&(TH_info_recv_buf.pData), 0, sizeof(ProcessedData_s));
        memset(&VAEngineID, 0, sizeof(VAEngineID));
        memset(&mac_id, 0, sizeof(mac_id));

	rdk_logger_init(LOGGER_PATH);
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Testharness Constructor. \n",__FUNCTION__, __LINE__);
	meta_data = (ProcessedData_s*) malloc(sizeof(ProcessedData_s));
	if( NULL == meta_data ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Failed to allocate memory to meta_data.\n",__FUNCTION__,__LINE__);
	}
}

/* Destructor */
TestHarness::~TestHarness()
{
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Testharness Destructor. \n",__FUNCTION__, __LINE__);
        if(NULL != meta_data ) {
                free(meta_data);
                meta_data = NULL;
        }
}

int TestHarness::RdkCTHCheckConnStatus(const char *ifname)
{
    int status = -1;

    int socId = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (socId < 0){
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Socket creation failed:Errno(%d).\n",__FUNCTION__, __LINE__, errno);
        return RDKC_FAILURE;
    }

    struct ifreq if_req;
    strncpy(if_req.ifr_name, ifname, IFNAMSIZ - 1);
    if_req.ifr_name[IFNAMSIZ] = '\0';

    do {
        int retVal = ioctl(socId, SIOCGIFFLAGS, &if_req);

        if (retVal == -1){
	       RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Socket creation failed:Errno(%d).\n",__FUNCTION__, __LINE__, errno);
               close(socId);
               return RDKC_FAILURE;
	}

        status = (if_req.ifr_flags & IFF_UP) && (if_req.ifr_flags & IFF_RUNNING);

	if(!status){
               RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Requested interface %s is not up or running. Rechecking after 10 sec.\n",__FUNCTION__, __LINE__,ifname);
               sleep(10);
	}
    }while(!status);

    close(socId);
    return RDKC_SUCCESS;
}
/**
 * @brief This function is used to set VA Engine.
 * @param char.
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHSetVAEngine(const char *VAEngine)
{
	memset(VAEngineID,0,MINSIZE+1);
	if( NULL == VAEngine ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): input argument is null\n",__FUNCTION__, __LINE__);
		return RDKC_FAILURE;
	}
	strncpy(VAEngineID,VAEngine,MINSIZE);
	VAEngineID[MINSIZE] = '\0';
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Video-Analytics Engine : %sOK\n",__FUNCTION__, __LINE__,VAEngineID);
        return RDKC_SUCCESS;
}

/**
 * @brief This function is used to set Image resolution
 * @param width : width of image
 * @param height : height of image
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHSetImageResolution(int width, int height)
{
	if( width < 0 || height < 0 ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): invalid width or height \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
	}
	imageWidth = width;
	imageHeight = height;
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to set number of detected objects
 * @param int.
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHSetObjectsCount(int objCount)
{
	if( objCount < 0 || objCount > 32 ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): invalid object count\n",__FUNCTION__, __LINE__);
		return RDKC_FAILURE;
	}
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): seting the object count as : %d\n",__FUNCTION__,__LINE__,objCount);
        objectCounter = objCount;
        return RDKC_SUCCESS;
}

/**
 * @brief This function is used to set number of detected events
 * @param int.
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHSetEventsCount(int evtCount)
{
	if( evtCount < 0 || evtCount > 32 ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): invalid event count\n",__FUNCTION__, __LINE__);
		return RDKC_FAILURE;
        }
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): seting the event count as : %d\n",__FUNCTION__,__LINE__,evtCount);
        eventCounter = evtCount;
        return RDKC_SUCCESS;
}

/**
 * @brief This function is used to initialize test-harness module
 * @param void.
 * @return RDKC_SUCCESS on success, RDKC_FAILURE on failure.
 */
int TestHarness::RdkCTHInit()
{
	char hostname[MAXSIZE+1] = {0};
	int port = 0;
	configParam = (char*) malloc(MAXSIZE);
	if( NULL == configParam ) {
        	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Could not allocate memory for configParam. \n",__FUNCTION__, __LINE__);
	        return RDKC_FAILURE;
    	}
	if (config_init() < 0) {
        	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Config Manager cannot be initialized.\n", __FUNCTION__, __LINE__);
	        return RDKC_FAILURE;
    	}
	/* Reading hostname from configuration file */
	configParam = (char*) rdkc_envGet(HOSTNAME);
	if( NULL != configParam ) {
		strncpy(hostname,configParam,MAXSIZE);
		hostname[MAXSIZE] = '\0';
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Successfully got parameter hostname\n", __FUNCTION__, __LINE__);
    	} else {
        	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Unable to get Requested Parameter hostname.\n", __FUNCTION__, __LINE__);
	        return RDKC_FAILURE;
    	}
	/* Reading port from configuration file */
        configParam = (char*) rdkc_envGet(PORT);
        if( NULL != configParam ) {
		port = strtol(configParam,NULL,10);
                RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Successfully got parameter port\n", __FUNCTION__, __LINE__);
        } else {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Unable to get Requested Parameter port.\n", __FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
	client_ptr = new SocketClient(hostname, port);
	if(!client_ptr->Connect()){
    		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to connect to server.\n",__FUNCTION__, __LINE__);
        	return RDKC_FAILURE;
	}
	/* Reading Test_harness_on_file_enables from configuration file */
	configParam = (char*) rdkc_envGet(TEST_HARNESS_ON_FILE_ENABLED);
	if( NULL != configParam ) {
		memset(enableTestHarnessOnFileFeed,0,sizeof(enableTestHarnessOnFileFeed));
		strncpy(enableTestHarnessOnFileFeed,rdkc_envGet(TEST_HARNESS_ON_FILE_ENABLED),MINSIZE);
		enableTestHarnessOnFileFeed[MINSIZE] = '\0';
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Successfully got parameter for file feed.\n", __FUNCTION__, __LINE__);
	} else {
        	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Unable to get Requested Parameter for file feed.\n", __FUNCTION__, __LINE__);
	        return RDKC_FAILURE;
    	}

	/* Reading Reset_Alg_on_First_Frame from configuration file */
	configParam = (char*) rdkc_envGet(RESET_ALG_ON_FIRST_FRAME);
	if( NULL != configParam ) {
		memset(resetAlgOnFirstFrame,0,sizeof(resetAlgOnFirstFrame));
		strncpy(resetAlgOnFirstFrame,rdkc_envGet(RESET_ALG_ON_FIRST_FRAME),MINSIZE);
		resetAlgOnFirstFrame[MINSIZE] = '\0';
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Successfully got parameter for file feed.\n", __FUNCTION__, __LINE__);
	} else {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Unable to get Requested Parameter for file feed.\n", __FUNCTION__, __LINE__);
		return RDKC_FAILURE;
	}

	/* Reading Testharness_with_ROI from configuration file */
	configParam = (char*) rdkc_envGet(TEST_HARNESS_WITH_ROI);
	if( NULL != configParam ) {
		memset(enableTestHarnessWithROI,0,sizeof(enableTestHarnessWithROI));
		strncpy(enableTestHarnessWithROI,rdkc_envGet(TEST_HARNESS_WITH_ROI),MINSIZE);
		enableTestHarnessWithROI[MINSIZE] = '\0';
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Successfully got parameter for ROI.\n", __FUNCTION__, __LINE__);
	} else {
        	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Unable to get Requested Parameter for ROI.\n", __FUNCTION__, __LINE__);
	        return RDKC_FAILURE;
    	}

	/* Reading Testharness_with_DOI from configuration file */
	configParam = (char*) rdkc_envGet(TEST_HARNESS_WITH_DOI);
	if( NULL != configParam ) {
		memset(enableTestHarnessWithDOI,0,MINSIZE);
        	strcpy(enableTestHarnessWithDOI,rdkc_envGet(TEST_HARNESS_WITH_DOI));
	        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Successfully got parameter for DOI.\n", __FUNCTION__, __LINE__);
	} else {
        	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Unable to get Requested Parameter for DOI.\n", __FUNCTION__, __LINE__);
	        return RDKC_FAILURE;
    	}

        if(!client_ptr->SendServerVersion()) {
                RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send server version.\n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }

	/* Inform server about MAC Id */
	RdkCTHGetMACId();
	if(!client_ptr->SendMacId(mac_id)) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send MAC id.\n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }

	/* Inform server abount client type */
	if( strcmp(enableTestHarnessOnFileFeed,"true") == 0) {
		if(!client_ptr->SendClientType(TH_VIDEO)) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send client type.\n",__FUNCTION__, __LINE__);
			return RDKC_FAILURE;
		}
	}
	else {
		if(!client_ptr->SendClientType(TH_CAMERA)) {
                        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send client type.\n",__FUNCTION__, __LINE__);
                        return RDKC_FAILURE;
                }
	}
	
	/* Inform server about algorithm type */
	if(strcmp(VAEngineID,"IV") == 0 ) {
		if(!client_ptr->SendAlgorithm(TH_ALG_IV_ENGINE)) {
                        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send algorithm.\n",__FUNCTION__, __LINE__);
                        return RDKC_FAILURE;
                }
	}
	else if(strcmp(VAEngineID,"RDKCVA") == 0 ) {
		if(!client_ptr->SendAlgorithm(TH_ALG_RDKC_ENGINE)) {
                        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send algorithm.\n",__FUNCTION__, __LINE__);
                        return RDKC_FAILURE;
                }
	}
	else {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): not able to sent Algorithm.\n",__FUNCTION__, __LINE__);
		return RDKC_FAILURE;
	}

	if( strcmp(enableTestHarnessOnFileFeed,"false") == 0) {
		if(!client_ptr->SendImageResolution(imageWidth, imageHeight)) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send width and height to TH Server.\n",__FUNCTION__, __LINE__);
			return RDKC_FAILURE;
		}
	}
	if(!client_ptr->ReceiveClipSize(&clipSize)) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send width and height to TH Server.\n",__FUNCTION__, __LINE__);
		clipSize = CLIP_DURATION;
		return RDKC_FAILURE;
        }

	if( strcmp(enableTestHarnessWithROI,"false") == 0) {
		if(!client_ptr->SendROIState(false)) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send ROI state to TH Server.\n",__FUNCTION__, __LINE__);
			return RDKC_FAILURE;
		}
	} else {
		if(!client_ptr->SendROIState(true)) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send ROI state to TH Server.\n",__FUNCTION__, __LINE__);
			return RDKC_FAILURE;
		}
        }

	if( strcmp(enableTestHarnessWithDOI,"false") == 0) {
		if(!client_ptr->SendDOIState(false)) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send DOI state to TH Server.\n",__FUNCTION__, __LINE__);
			return RDKC_FAILURE;
		}
	} else {
		if(!client_ptr->SendDOIState(true)) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): unable to send DOI state to TH Server.\n",__FUNCTION__, __LINE__);
			return RDKC_FAILURE;
		}
        }

	pthread_create(&THUploadThread, NULL, ThraedFunction, this);
	pthread_setname_np(THUploadThread,"TH_upload_thraed");
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to get mac id of the device
 * @param void.
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHGetMACId()
{
    int s;
    struct ifreq buffer;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Socket creation failed:Errno(%d).\n",__FUNCTION__, __LINE__, errno);
        return RDKC_FAILURE;
    }
    memset(&buffer, 0x00, sizeof(buffer));
    strncpy(buffer.ifr_name, (char*) rdkc_envGet(NW_INTERFACE),IFNAMSIZ - 1);
    buffer.ifr_name[IFNAMSIZ] = '\0';
    ioctl(s, SIOCGIFHWADDR, &buffer);
    close(s);
    memset(mac_id,0,MACSIZE);

    for( s = 0; s < 5; s++ )
    {
	char temp[3];
        sprintf(temp,"%.2X:", (unsigned char)buffer.ifr_hwaddr.sa_data[s]);
	strcat(mac_id,temp);
    }
	char temp[2];
        sprintf(temp,"%.2X", (unsigned char)buffer.ifr_hwaddr.sa_data[s]);
	strcat(mac_id,temp);
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): MAC Id : %s\n",__FUNCTION__,__LINE__,mac_id);
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to reset payload
 * @param void.
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHResetPayload()
{
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): Reseting the payload\n",__FUNCTION__,__LINE__);
	objectCounter = 0;
	eventCounter = 0;
	if( NULL == meta_data ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Inavlid memory pointer for obj or evt or meta_data\n",__FUNCTION__,__LINE__);
		return RDKC_FAILURE;
	}
	memset(meta_data, 0, sizeof(ProcessedData_s));
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to create payload
 * @param objCount : number of objects detected
 * @param evtCount : number of events detected
 * @param th_objects : objects
 * @param th_events : events
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHCreatePayload(int objCount,int evtCount,th_iObject *th_objects,th_iEvent *th_events, bool th_isMotionInsideROI, bool th_isMotionInsideDOI)
{
	RdkCTHSetObjectsCount(objCount);
	RdkCTHSetEventsCount(evtCount);
	RdkCTHSetObject(th_objects);
	RdkCTHSetEvent(th_events);

	if( NULL == meta_data ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Inavlid memory pointer for obj or evt or meta_data\n",__FUNCTION__,__LINE__);
                return RDKC_FAILURE;
        }
	meta_data -> objectCount = objectCounter;
	meta_data -> eventCount = eventCounter;
	meta_data -> isMotionInsideROI = th_isMotionInsideROI;
	meta_data -> isMotionInsideDOI = th_isMotionInsideDOI;

	if( strcmp(enableTestHarnessOnFileFeed,"false") == 0) {
		TH_info.frame = RGBMat;
		TH_info.pData = *meta_data;
	}
	else if( strcmp(enableTestHarnessOnFileFeed,"true") == 0) {
                TH_info.frame.release();
                TH_info.pData = *meta_data;
        }

	std::unique_lock<std::mutex> lock(QueueMutex);
	THInfoQue.push(TH_info);
	lock.unlock();

	return RDKC_SUCCESS;
}

void* ThraedFunction(void* args)
{
        int ret = RDKC_SUCCESS ;
	TestHarness* th_ptr = (TestHarness*) args;
	ret = th_ptr -> RdkCTHUploadPayload();
        if( RDKC_FAILURE == ret ) {
            std::cout << "Exiting after RdkCTHUploadPayload error" <<  std::endl;
            pthread_exit(NULL);
        }
	return NULL;
}

/**
 * @brief This function is used to upload payload
 * @param void.
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHUploadPayload()
{
	cout <<"Thread has been created"<<endl;
	while(1) {
		if( !THInfoQue.empty() ) {
			std::unique_lock<std::mutex> lock(QueueMutex);
			TH_info_recv_buf = THInfoQue.front();
			THInfoQue.pop();
			lock.unlock();
			/* Send Image if the frame is from camera */
			if( strcmp(enableTestHarnessOnFileFeed,"false") == 0) {
				if(!client_ptr -> SendImage(TH_info_recv_buf.frame)){
					std::cout << "Error sending image" <<  std::endl;
				        return RDKC_FAILURE;
        			}
			}

                        if(fileEnd) {
                            sleep(15); //Wait for last delivery result in case of File End
			    fileEnd = false;
                        }

			if( NULL != meta_data ) {
                            if( !DeliveryInfoQue.empty() ) {
                                std::unique_lock<std::mutex> lock(DQueMutex);
                                th_deliveryResult result = DeliveryInfoQue.front();
                                DeliveryInfoQue.pop();
                                lock.unlock();
			        if(!client_ptr -> SendDeliveryResult(result)) {
				    std::cout << "Error sending delivery result"  <<  std::endl;
				    return RDKC_FAILURE;
			        }
                            }
			    if(!StnUploadStatusQue.empty()){
				th_smtnUploadStatus stnStatus=StnUploadStatusQue.front();
				if(!client_ptr->SendStnUploadStatus(stnStatus)){
				    std::cout<<"Error sending smartThumbnail UploadStatus"<< std::endl;
			    	}
			    }
			    if(!client_ptr -> SendTestHarnessMetaData(&TH_info_recv_buf.pData)) {
				std::cout << "Error sending meta_data" <<  std::endl;
				return RDKC_FAILURE;
			    }
			}
			else {
				RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Inavlid memory pointer for meta_data\n",__FUNCTION__,__LINE__);
				return RDKC_FAILURE;
			}
		}
		else {
			usleep(10);
		}
	}
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to get farme from the server.
 * @param plane0 is y plane
 * @param plane1 is uv plane
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHGetFrame(th_iImage *plane0, th_iImage *plane1, int &fps)
{
	int image_header = RDKC_SUCCESS;
	image_header = (client_ptr->ReceiveImage(fileFrameBGR, fps));

        /* TODO: The metadata of each file will be send to the camera before the first frame
         * of each file. The metadata should be send with a header H_FILE_METADATA. Inside the
         * file metadata, we can add different types of metadata(for future use) with separate
         * headers. Here, we are sending only one type of metadata, the ROI coordinates. So, just
         * added H_ROI headeronly. */
        if(image_header == H_ROI) {
            isROICoordsChanged = true;
            roiCoords = client_ptr->ReceiveROI();
            image_header = (client_ptr->ReceiveImage(fileFrameBGR, fps));
        } 
        if(image_header == H_DOI) {
	    isDOIChanged = true;
	    doi_enable = true;
	    client_ptr->ReceiveDOI(doiThreshold);
            image_header = (client_ptr->ReceiveImage(fileFrameBGR, fps));
        }

        if((image_header == H_FSTART) && (!isROICoordsChanged)) {
            isROICoordsChanged = true;
            roiCoords = "0";
        }

        if((image_header == H_FSTART) && (!isDOIChanged)) {
            isDOIChanged = true;
            doi_enable = false;
        }

	if ( image_header != H_IMAGE && image_header != H_FSTART && image_header != H_FEND) {
		cout << " No image received" <<endl;
		return RDKC_FAILURE;
	}
        	cout << "received image" << endl;


	if( !fileFrameBGR.empty() ) {
		cvtColor(fileFrameBGR, fileFrameYUV, COLOR_BGR2YUV);
		th_frame_width = fileFrameYUV.cols;
		th_frame_height = fileFrameYUV.rows;
		resize(fileFrameYUV, fileFrameYUV, Size(DEFAULT_WIDTH, DEFAULT_HEIGHT));
		split(fileFrameYUV, yuvPlanes);

		yuvChannels.clear();

		yuvChannels.push_back(yuvPlanes[1]);
		yuvChannels.push_back(yuvPlanes[2]);

		merge(yuvChannels,planeUV);

		plane0 -> data = yuvPlanes[0].data;
		plane0 -> size = yuvPlanes[0].cols * yuvPlanes[0].rows;
		plane0 -> width = yuvPlanes[0].cols;
		plane0 -> height = yuvPlanes[0].rows;
		plane0 -> step = 0;

		plane1 -> data = planeUV.data;
		plane1 -> size = (planeUV.cols * planeUV.rows)/2;
		plane1 -> width = planeUV.cols;
		plane1 -> height = planeUV.rows;
		plane1 -> step = 0;

	}
	else if( fileFrameBGR.empty() ) {
	        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Frame is empty!!!! \n", __FUNCTION__, __LINE__);
        	return RDKC_FAILURE;
	}

	if( H_FSTART == image_header) {
		return RDKC_FILE_START;
	}

	if( H_FEND == image_header) {
                fileEnd = true;
		return RDKC_FILE_END;
	}
	return RDKC_SUCCESS;

}

/**
 * @brief This function is used to Fill frame data into the TH structure.
 * @param plane0 is y plane
 * @param plane1 is uv plane
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHPassFrame(th_iImage *plane0, th_iImage *plane1)
{
	cv::Mat yuvMat;
        frame.y_data = plane0->data;
        frame.y_size = plane0->size;
        frame.y_height = plane0->height;
        frame.y_width = plane0->width;
        frame.uv_data = plane1->data;
        frame.uv_size = plane1->size;
        frame.uv_height = plane1->height;
        frame.uv_width = plane1->width;

	if(!yuvDataMemoryAllocationDone) {
		yuvData = (unsigned char *) malloc((frame.y_size + frame.uv_size) * sizeof(unsigned char));
		yuvDataMemoryAllocationDone = true;
	}
	if(NULL == yuvData ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Invalid pointer to yuvData\n",__FUNCTION__,__LINE__);
		return RDKC_FAILURE;
	}

	memset( yuvData, 0, (frame.y_size + frame.uv_size) * sizeof(unsigned char));
	memcpy( yuvData, frame.y_data,frame.y_size);
	memcpy( yuvData+frame.y_size,frame.uv_data,frame.uv_size );
	yuvMat = cv::Mat( frame.y_height + frame.y_height/2, frame.y_width, CV_8UC1, yuvData );


        /* matrix to store color image */
        RGBMat = cv::Mat( frame.y_height, frame.y_width, CV_8UC4);
        /* convert the frame to BGR format */
        cv::cvtColor(yuvMat, RGBMat, CV_YUV2BGR_NV12);

	//if(strcmp(VAEngineID,"RDKCVA") == 0 ) {
		/* resize the frame to 360x202 */
	        //float resize_factor = 360.0/RGBMat.cols; // resize to width =360
	        //cv::resize(RGBMat,RGBMat,cv::Size(),resize_factor,resize_factor);
	//}


        return RDKC_SUCCESS;
}

/**
 * @brief This function is used to rescale the blob data to original frame size.
 * @param object object structure
 */
void TestHarness::RdkCTHRescaleBlob(Object_s *object)
{
	object->lcol = ((float)object->lcol / DEFAULT_WIDTH) * th_frame_width;
	object->rcol = ((float)object->rcol / DEFAULT_WIDTH) * th_frame_width;
	object->trow = ((float)object->trow / DEFAULT_HEIGHT) * th_frame_height;
	object->brow = ((float)object->brow / DEFAULT_HEIGHT) * th_frame_height;
	return;
}

/**
 * @brief This function is used to set the object structure.
 * @param object th_object structure
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHSetObject(th_iObject *object)
{
	if( NULL == object ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): *obj or input argument is null. \n",__FUNCTION__, __LINE__);
		return RDKC_FAILURE;
	}
	th_iObject *th_object_ptr = object;

	int _objectCounter = objectCounter;
	int i = 0;
        while(_objectCounter-- > 0) {
		meta_data->objects[i].lcol = th_object_ptr->th_m_u_bBox.th_m_i_lCol;
                meta_data->objects[i].rcol = th_object_ptr->th_m_u_bBox.th_m_i_rCol;
                meta_data->objects[i].trow = th_object_ptr->th_m_u_bBox.th_m_i_tRow;
                meta_data->objects[i].brow = th_object_ptr->th_m_u_bBox.th_m_i_bRow;
                meta_data->objects[i].object_class = th_object_ptr->th_m_e_class;
		RdkCTHRescaleBlob(&(meta_data->objects[i]));
		i++;
		th_object_ptr++;
	}
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to get the clip size.
 * @param clip_size Clip size
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHGetClipSize(int32_t *clip_size)
{
        if( NULL == clip_size ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): *clip_size or input argument is null. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
	*clip_size = clipSize;
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to set the event structure.
 * @param event th_event structure
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHSetEvent(th_iEvent *event)
{
	if( NULL == event ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): *evt or input argument is null. \n",__FUNCTION__, __LINE__);
                return RDKC_FAILURE;
        }
	th_iEvent *th_event_ptr = event;
	int _eventCounter = eventCounter;
	int i = 0;
        while(_eventCounter-- > 0) {
		meta_data->events[i].event_type = th_event_ptr -> th_m_e_type;
                th_event_ptr++;
		i++;
        }
	return RDKC_SUCCESS;
}

bool TestHarness::RdkCTHIsROICoordinatesChanged()
{
        return isROICoordsChanged;
}

std::string TestHarness::RdkCTHGetROICoordinates()
{
	isROICoordsChanged = false;
        return roiCoords;
}

bool TestHarness::RdkCTHIsDOIChanged()
{
	return isDOIChanged;
}

bool TestHarness::RdkCTHGetDOIConfig(int &threshold)
{
	threshold = doiThreshold;
	isDOIChanged = false;
	return doi_enable;
}

int TestHarness::RdkCTHWriteFrame(std::string filename)
{
	if( fileFrameBGR.empty() ) {
        	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.TESTHARNESS","%s(%d): fileFrameBGR.empty()\n",__FUNCTION__, __LINE__);
		return RDKC_FAILURE;
	}
	imwrite(filename.c_str(), fileFrameBGR);
	return RDKC_SUCCESS;
}

int TestHarness::RdkCTHAddDeliveryResult(th_deliveryResult result)
{
	//Add delivery result to queue
	std::unique_lock<std::mutex> lock(DQueMutex);
	DeliveryInfoQue.push(result);
	lock.unlock();
	return RDKC_SUCCESS;
}

int TestHarness::RdkCTHAddSTNUploadStatus(th_smtnUploadStatus stnStatus)
{
	StnUploadStatusQue.push(stnStatus);
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to release occupied memory
 * @param void
 * @return RDKC_SUCCESS on success.
 */
int TestHarness::RdkCTHRelease()
{
	/* Close the socket connection */
	delete client_ptr;
	client_ptr = NULL;
	return RDKC_SUCCESS;
}

#endif
