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

#include "TestHarnessServer.hpp"

#define IMAGE_RAW 202
#define IMAGE_COL 360

#define MAXSIZE 100                     /* maximum size for array */
#define STR_LENGTH 100
#define BOX_THICKNESS 2                 /* box thickness */
#define TEXT_THICKNESS 1                /* text thickness */
#define FONT_SCALE 0.5                  /* font scale */
#define X 20                            /* starting x-cordinate */
#define Y 20                            /* starting y-cordinate */
#define SHIFT 20                        /* shift in cordinate */
#define STR_LENGTH 100                  /* max string length */
#define EVENT_LENGTH 100                /* max event length */

#define FRAME_LIMIT 50

#define OBJECT_M_E_CLASS "object_m_e_class"
#define OBJECT_M_I_LCOL "object_m_i_lcol"
#define OBJECT_M_I_RCOL "object_m_i_rcol"
#define OBJECT_M_I_TROW "object_m_i_trow"
#define OBJECT_M_I_BROW "object_m_i_brow"

#define EVENT_M_E_TYPE  "event_m_e_type"

#define OBJECT_EVENT_INFO "object_event_info"
#define DELIVERY_RESULTS_INFO "delivery_results_info"
#define OBJECT_INFO "object_info"
#define EVENT_INFO "event_info"

#define FRAME_COUNTER "frame_count"
#define TIME_STAMP "time_stamp"
#define ROI_MOTION "roi_motion"
#define DOI_MOTION "doi_motion"
#define JSON_FILENAME "filename"
#define JSON_FILE_TIMESTAMP "file timestamp"
#define JSON_FILE_END_TIMESTAMP "file end timestamp"
#define JSON_FILE_PROCESS_TIME "file processing time(in sec)"

#define DEFAULT_ENCODING_FORMAT  "h264"
#define DEFAULT_OUTPUT_VIDEO_DIR "/tmp"
#define DEFAULT_SOURCE_VIDEO_DIR "/tmp"
#define DEFAULT_FARME_LIMIT 100
#define DEFAULT_PORT 9000
#define CLIP_DURATION 16

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define CONF_FILE_SUFFIX ".conf"
#define DD_RESULT_FILE "DeliveryDetectionResults.json"


/* Specific parameters to displaying objects and event and timestamp on the frame */
//Object
cv::Scalar textColorForObj(255, 0, 255); //magenta
cv::Scalar boxColorForUnknown(0, 255, 255); //yellow
cv::Scalar textColorForUnknown(0, 255, 255); //yellow
cv::Scalar boxColorForHuman(0, 255, 0); //green
cv::Scalar textColorForHuman(0, 255, 0); //green
cv::Scalar boxColorForVehicle(0, 0, 128); //maroon
cv::Scalar textColorForVehicle(0, 0, 128); //maroon
cv::Scalar boxColorForTrain(19,69,139); //brown
cv::Scalar textColorForTrain(19,69,139); //brown
cv::Scalar boxColorForPet(226,43,138); //violet
cv::Scalar textColorForPet(226,43,138); //violet
cv::Point textOrgForObj(X,Y);
cv::Point textOrgForHuman(X,Y+1*SHIFT);
cv::Point textOrgForVehicle(X,Y+2*SHIFT);
cv::Point textOrgForTrain(X,Y+3*SHIFT);
cv::Point textOrgForPet(X,Y+4*SHIFT);
cv::Point textOrgForUnknown(X,Y+5*SHIFT);

//Event
cv::Scalar textColorForEventCount(255, 255, 0); //cyan
cv::Scalar textColorForEvent(255,0,255); //magenta

//Time
cv::Scalar textColorForTime(0, 0, 0); //black

//ROI Motion
cv::Scalar textColorForROIMotion(255,0,255); //magenta
cv::Scalar textColorForDOIMotion(0,0,128); //maroon

THServer *THServer::thServerInstance = 0;

THServer* THServer::THCreateServerInstance(){
    if(!thServerInstance){
        thServerInstance = new THServer();
    }
    return thServerInstance;
}


THServer::THServer():
                 port(DEFAULT_PORT),
                 ss(NULL),
                 clientType(TH_UNKNOWN),
                 outputVideoDir(DEFAULT_OUTPUT_VIDEO_DIR),
                 archive_processed(true),
                 encodingFormat(DEFAULT_ENCODING_FORMAT),
                 srcVideoPath(DEFAULT_SOURCE_VIDEO_DIR),
                 sourceVideoDir(NULL),
                 videoFile(NULL),
                 frameCtr(0),
                 frameNum(0),
                 fileNum(0),
                 frameCountLimit(DEFAULT_FARME_LIMIT),
                 processingDataInProgress(false),
                 fileChangeStatus(true),
                 firstFrame(true),
                 supportFrameDisplay(false),
                 resizeFrame(false),
		 segmentation_size(CLIP_DURATION),
                 isRoiEnabled(true),
                 isDoiEnabled(true),
		 outputProcessedVideo(false),
		 raw_video(false),
		 ImageRows(0),
		 ImageCols(0){}

/**
* @brief  The function trims the blank spaces from string 
* @param  str, Text to be trimmed
* @return Trimmed line
*/

std::string THServer::THTrim(const std::string& str)
{
    const std::string whitespaces (" \t\f\v\n\r");

    size_t first = str.find_first_not_of(whitespaces);

    if (std::string::npos == first)
    {
        return "";
    }
    size_t last = str.find_last_not_of(whitespaces);
    return str.substr(first, (last - first + 1));
}

/**
* @brief  The function initialize the parameters required by THServer 
* @param  configFilePath, Path to THServerConfigFile. 
* @return true on success, false otherwise 
*/
bool THServer::THServerInit(std::string configFilePath){
    FILE *fileDesc = NULL;
    char lineBuff[255] = {0};
    size_t equalsIndex = -1;

    std::string paramValuePair;
    std::string param;
    std::string value;
    
    if((fileDesc = fopen(configFilePath.c_str(),"r")) == NULL){
        std::cout << "Error opening THServerConfig.txt" << std::endl;
        return false;
    }

    while(fgets(lineBuff, sizeof(lineBuff), fileDesc) != NULL){

        if (lineBuff[0] == '#')
            continue;

        paramValuePair = lineBuff;
        
        if ((equalsIndex = paramValuePair.find('=')) == std::string::npos)
            continue;

        param = THTrim(paramValuePair.substr(0,equalsIndex));
        if(param.empty())
            continue;
    
        value = THTrim(paramValuePair.substr((equalsIndex+1), std::string::npos));
        if(value.empty())
            continue;

        if(param.compare("port") == 0){
            port = atoi(value.c_str());
        }
        else if (param.compare("frame_limit") == 0){
            //frameCountLimit = atoi(value.c_str());
            frameCountLimit = strtoul (value.c_str(), NULL, 0);
        }
        else if (param.compare("source_video_dir") == 0){
            srcVideoPath = value;
        }
        else if (param.compare("output_video_dir") == 0){
            outputVideoDir = value;
        }
        else if (param.compare("encoding_format") == 0){
            encodingFormat = value;
        }
        else if (param.compare("support_imshow") == 0 ){
            if( value.compare("true") == 0){
                supportFrameDisplay = true;
            } else { 
                supportFrameDisplay = false;
            }
        }
        else if (param.compare("resize_frame") == 0 ){
            if( value.compare("true") == 0){
                resizeFrame = true;
            } else {
                resizeFrame = false;
            }
        }
	else if (param.compare("output_processed_video") == 0){
	    if( value.compare("true") == 0){
                outputProcessedVideo = true;
            } else {
                outputProcessedVideo = false;
            }
	}
	else if (param.compare("resize_resolution") == 0 ){
	    sscanf(value.c_str(),"%dx%d",&ResizedImageColumn,&ResizedImageRow);
	}
	else if (param.compare("raw_video") == 0 ){
	    if( value.compare("true") == 0){
                raw_video = true;
            }
	}
	else if (param.compare("archive_processed") == 0 ){
	    if( value.compare("true") == 0){
                archive_processed = true;
            } else {
                archive_processed = false;
            }
	}
        else if (param.compare("segmentation_size") == 0 ){
             segmentation_size = std::stoi(value);
        }
        else{
            std::cout << "Cannot initialize Parameter:" <<  param  << std::endl;
        }
     }
    
     return true;
}

/**
* @brief  This function is used to start the Server and spawn processing Thread 
*         and Video file Monitor thread 
* @param  void
* @return true on success, false otherwise
*/
bool THServer::THServerStart(){

    ss = new (std::nothrow) SocketServer(port);
    
    if(ss == nullptr){
        std::cout << "Unable to allocate memory" << std::endl;
        return false;
    }

    if(!ss->Connect())
        return false;

    std::cout << "Listening on port:" << port << std::endl;

    if(!ss->MonitorRequest())
        return false;

    msg = {};
    msg.msgType = TH_CLIENT_INFO;

    server_version = (enum ServerVersion)ss -> ReceiveServerVersion();
    std::cout << "Server Version: " << server_version << std::endl;

    msg.clientInfo.mac = ss -> ReceiveMACId();
    //std::cout << "MAC ID: " <<  msg.clientInfo.mac << std::endl;

    clientMAC = msg.clientInfo.mac;
     
    if(supportFrameDisplay)
        cv::namedWindow(clientMAC,1);

    msg.clientInfo.outputDir = outputVideoDir;

    clientType = msg.clientInfo.clientType = ss->ReceiveClientType();
    //std::cout << "Client type: " << msg.clientInfo.clientType << std::endl;

    if(msg.clientInfo.clientType == TH_UNKNOWN){
        std::cout << "Error Identifying the client" << std::endl;
        return false;
    }

    msg.clientInfo.algType = ss-> ReceiveAlg();
    //std::cout << "Algorithm type: " << msg.clientInfo.algType << std::endl; 
    if( clientType == TH_CAMERA) {
        ss-> ReceiveImageResolution(&ImageCols, &ImageRows);
    }

    ss->SendClipSize(segmentation_size);

    if(server_version > TH_VERSION_1_2) {
        isRoiEnabled = ss-> ReceiveROIState();
    }

    if(server_version >= TH_VERSION_DOI) {
        isDoiEnabled = ss-> ReceiveDOIState();
    }

    if(msg.clientInfo.algType == TH_ALG_UNKNOWN){
        std::cout << "Error Identifying the algoritm used" << std::endl;
        return false;
    }

    if(archive_processed) {
        THArchiveProcessedFiles();
    }

    if(clientType == TH_VIDEO) {
    THGetProcessedVideos();

    if(TH_FAILURE == THPopulateVideoList())
        return false;

    videoFileMonitor = std::thread(&THServer::THMonitorVideoFile, this);
    }

    frameProcessThread = std::thread(&THServer::MessageQueueThreadFunc,this);

    THPushMessage(msg);
   
    return true;
    
}

/**
* @brief  This function is used to archive already processed videos
* @param  void
* @return None
*/
void THServer::THArchiveProcessedFiles()
{
    char videoTimeStamp[MAXSIZE] = {0};
    size_t num_bytes = 0;
    std::string cmd;

    cmd.clear();
    cmd = "mkdir -p " + outputVideoDir + "/Backups";
    std::cout << "about to execute Command:" <<  cmd << std::endl;
    system(cmd.c_str());

    cmd.clear();
    time (&timestamp);
    timeinfo = localtime (&timestamp);
    if((num_bytes = strftime(videoTimeStamp,50,"%m%d%H%M%S",timeinfo)) == 0){
        std::cout << "Error generating Timestamp" << std::endl;
        return;
    }

    system(cmd.c_str());
    cmd = "sh -c \" cd ";
    cmd += outputVideoDir;
    cmd+= " && tar cvf Backups/";
    cmd += clientMAC;
    cmd += "-";
    cmd += videoTimeStamp;
    cmd += ".tar ";
    cmd += clientMAC;
    cmd += " \"";
    std::cout << "about to execute Command:" <<  cmd << std::endl;
    system(cmd.c_str());

    cmd.clear();
    cmd = "rm -rf " + outputVideoDir + "/" + clientMAC;
    std::cout << "about to execute Command:" <<  cmd << std::endl;
    system(cmd.c_str());
}

/**
* @brief  This function is used to retrive already processed videos
* @param  void
* @return None
*/
void THServer::THGetProcessedVideos()
{
    std::string clientDirPath;
    FILE *fileDesc = NULL;
    char filenameBuff[50] = {0};

    clientDirPath = outputVideoDir+ "/" + clientMAC + "/videoInfo.txt";

    if((fileDesc = fopen(clientDirPath.c_str(),"r")) == NULL){
        return;
    }

    else{
        while(fgets(filenameBuff, sizeof(filenameBuff),  fileDesc) != NULL){
            filenameBuff[strlen(filenameBuff) -1] = '\0';
            recurringVideos.insert(filenameBuff);
        }
        if (feof(fileDesc)){
            fclose(fileDesc);
            return;
        }
    }
}

/**
* @brief  This function is used to retrive videos to be processedfrom the source directory
* @param  void
* @return TH_SUCCESS on success, TH_FAILURE in all other cases
*/
Status THServer::THPopulateVideoList()
{
    const char period = '.';
    std::size_t periodPos;

    std::string filename;
    //const std::string eligibleExt(".avi");
    std::string fileExt;

    struct stat fileStat; 

    videoList.empty();

    if((sourceVideoDir = opendir(srcVideoPath.c_str())) ==NULL){
        std::cout << "Error opening the source directory" << std::endl;
        return TH_FAILURE;
    }

    std::string devMac = clientMAC;
    char videoListFile[MAXSIZE];

    //Remove ':' from MAC
//    devMac.erase(remove(devMac.begin(), devMac.end(), ':'), devMac.end());

    sprintf(videoListFile, "%s/%s", srcVideoPath.c_str(), devMac.c_str());

    if (0 == stat(videoListFile, &fileStat)) {
        printf("Reading filelist from %s...\n", videoListFile);

        std::ifstream infile(videoListFile);

        std::string line;
        printf ("File list:");
        while (std::getline(infile, line)) {
            //Remove space from the line.
            line.erase(remove(line.begin(), line.end(), ' '), line.end());
            std::stringstream ss(line);

            while (std::getline(ss, filename, ',')) {
                if (0 == stat((srcVideoPath+"/"+filename).c_str(), &fileStat)) {
                    printf(" %s,", filename.c_str());
                    videoList.push(filename);
                }
            }
        }
        printf("\n");
    } else {
        printf("Video list file %s not present. Falling back to old file list population method\n", videoListFile);
        while((videoFile = readdir(sourceVideoDir)) != NULL){
            filename = videoFile->d_name;

            if((filename.compare(".") == 0) || (filename.compare("..") == 0) || (filename.find(CONF_FILE_SUFFIX) != std::string::npos))
                continue;

            if((periodPos = filename.rfind(period)) != std::string::npos)
                fileExt = filename.substr(periodPos);

            if (0 == stat((srcVideoPath+"/"+filename).c_str(), &fileStat) && (0 == fileStat.st_size))
                continue;

            if(recurringVideos.find(filename.substr(0, periodPos)) == recurringVideos.end()){
                videoList.push(filename);
            }
        }
    }

    if( videoList.size() !=0)
        std::cout << "Number of files to be processed:" << videoList.size() << std::endl;
    else
        std::cout << "No video file available to process in the source directory." << std::endl;

    return TH_SUCCESS;
}

/**
* @brief  This function is used to process the frames from video or camera feed  
* @param  void
* @return TH_SUCCESS on success, TH_FAILURE in all other cases
*/
bool THServer::THProcessFrames()
{
    if(clientType == TH_VIDEO){
        Status sendFrameStatus;
        while((sendFrameStatus = SendFrame()) == TH_RETRY);

        if(sendFrameStatus == TH_FAILURE)
            return false;
    }
    else if( clientType == TH_CAMERA && firstFrame == true){

        char videoTimeStamp[MAXSIZE] = {0};
        size_t num_bytes = 0;

        time (&timestamp);
        timeinfo = localtime (&timestamp);
        if((num_bytes = strftime(videoTimeStamp,50,"%m%d%H%M%S",timeinfo)) == 0){
            std::cout << "Error generating Timestamp" << std::endl;
            return false;
        }
        msg.msgType = TH_FILE_INFO;
        msg.fileInfo.name = "XCam-" + std::string(videoTimeStamp);
        msg.fileInfo.frameTotal = frameCountLimit;
        msg.fileInfo.fps = 10;
        msg.fileInfo.rows = ImageRows;
        msg.fileInfo.cols = ImageCols;
        THPushMessage(msg);
        firstFrame = false;
    }

    if(!ReceiveTHInfo())
        return false;
    return true;
}

/**
* @brief  This function is used to check if video files are available to be processed.
* @param  void
* @return TH_SUCCESS on success, TH_FAILURE in all other cases
*/
bool THServer::THVideoListIsEmpty()
{
    bool videoListEmpty = false;

    {
        std::unique_lock<std::mutex> lock(videoListMutex);
        videoListEmpty = videoList.empty();
        lock.unlock();
    }

    return videoListEmpty;
}

/**
* @brief  This function is used to send the frames from the video file to camera.
* @param  void
* @return TH_SUCCESS on success, TH_FAILURE in all other cases
*/
Status THServer::SendFrame()
{
    std::string fileBaseName;
    std::string currVideoFilename;
    std::string fullpath;
    std::string delimiter (".");
    std::size_t periodPos;
    std::string roiFileName;

    if(fileChangeStatus && !THVideoListIsEmpty()){

        msg = {};
        msg.msgType = TH_FILE_END;
    //    sleep(20);
        THPushMessage(msg);
        frameCtr = 0;

        {
        std::unique_lock<std::mutex> lock(videoListMutex);
        
        currVideoFilename = videoList.front();
        videoList.pop();

	processedFileList.push_back(currVideoFilename);

        lock.unlock();
        }

        if((periodPos = currVideoFilename.rfind(delimiter)) != std::string::npos) {
            fileBaseName = currVideoFilename.substr(0, periodPos); 
        }

        //search for the files in the list.
        fullpath = srcVideoPath + "/" + currVideoFilename;
        vCap.open(fullpath);

        if(!vCap.isOpened()) {
            std::cout << "Error opening file:" << currVideoFilename;
            std::cout << " ,Skipping to next video " << std::endl;
            return TH_RETRY;
        }
        if(!vCap.read(nextFrame)){
            std::cout << "Error reading frame from video" << std::endl;
            return TH_RETRY;
        }

        msg = {};

        if(resizeFrame){
        currFrame = nextFrame.clone();

        // resize
        cv::resize(currFrame,currFrame,cv::Size(ResizedImageColumn,ResizedImageRow));
        }

        msg.msgType = TH_FILE_INFO;
        msg.fileInfo.name = fileBaseName;
        msg.fileInfo.frameTotal = vCap.get(cv::CAP_PROP_FRAME_COUNT);
        msg.fileInfo.fps = vCap.get(cv::CAP_PROP_FPS);
	v_file_fps = msg.fileInfo.fps;
        if(resizeFrame){
            msg.fileInfo.rows = currFrame.rows;
            msg.fileInfo.cols = currFrame.cols;
        }else{
            msg.fileInfo.rows = vCap.get(cv::CAP_PROP_FRAME_HEIGHT);
            msg.fileInfo.cols = vCap.get(cv::CAP_PROP_FRAME_WIDTH);
        }

        THPushMessage(msg);

	    if((fileNum == 0) && (!ss->SetFileChangeStatus(true))) {
	        std::cout<<" Not able to set file change status"<<std::endl;
	    }
          fileNum++;
        fileChangeStatus = false;

	std::string confFname = srcVideoPath + "/" + fileBaseName + CONF_FILE_SUFFIX;
	if(!loadSourceFileConfig(confFname)) {
		std::cout << "Failed to load Source file config" << std::endl;
	}

        /* Send ROI coordinates before sending first frame of a file */
        if(isRoiEnabled) {
            //Send ROI coordinates even if it is empty
            ss->SendROI(config.roiCoords);
        }
        /* Send DOI */
        if(isDoiEnabled) {
	    ss->SendDOI(config.doi_threshold, config.doi_bitmap);
        }
    }
    else if(fileChangeStatus){
        msg = {};
        msg.msgType = TH_FILE_END;
        //sleep(2);
        THPushMessage(msg);
        sleep(20);
        std::cout << "Stopping processing..." << std::endl;
        return TH_FAILURE;
        std::cout << "Waiting for files..." << std::endl;
        {
            std::unique_lock<std::mutex> lock(videoListMutex);
            vListCv.wait(lock,[this]{return !videoList.empty();});

            lock.unlock();
        }

        return TH_RETRY;
    }

    //std::cout << "height:" << currFrame.rows << " " << currFrame.size().height << std::endl;
    //std::cout << "width:"  << currFrame.cols << " " << currFrame.size().width <<std::endl;

    currFrame = nextFrame.clone();
    if(resizeFrame){
        cv::resize(currFrame,currFrame,cv::Size(ResizedImageColumn,ResizedImageRow));
    }

    if(!vCap.read(nextFrame)){
        fileChangeStatus = true;
        if(!ss->SetFileChangeStatus(true)) {
            std::cout<<" Not able to set file change status"<<std::endl;
        }
    }
    if(!ss->SendImage(currFrame, v_file_fps))
        return TH_RETRY;

    if(!ss->SetFileChangeStatus(false)) {
        std::cout<<" Not able to set file change status"<<std::endl;
    }

    return TH_SUCCESS;
}

/**
* @brief  This function is used to receive metadata from the client
* @param  void
* @return TH_SUCCESS on success, TH_FAILURE in all other cases
*/
bool THServer::ReceiveTHInfo(){

    HeaderType htype = H_WAITING;
    msg = {};

    msg.msgType = TH_FRAME_INFO;

read_image:
    if(clientType == TH_CAMERA){
        if((htype = ss -> ReceiveImage(msg.frameInfo.image)) != H_IMAGE ) {
            if(htype == H_DDRESULT) {
		DeliveryResult_t result;
		if(!ss -> ReceiveDeliveryResult(&result)) {
		    std::cout << "Error receiving delivery result" << std::endl;
		} else {
                    std::cout << "Writing delivery result in xml" << std::endl;
                    RdkCTHAddDeliveryJSONFile(result);
                }
		goto read_image;
	    }
        }
        msg.frameInfo.frameIndex = ++frameCtr;
    }else{
        msg.frameInfo.image = currFrame;
        msg.frameInfo.frameIndex = ++frameCtr;
    }

    if(msg.frameInfo.image.empty()){
        std::cout << "Error frame not received" << std::endl;
        return false;
    }

read_processed_data:
    if((htype = ss -> ReceiveProcessedData(msg.frameInfo.meta_data)) != H_DATA){
        if(htype == H_DDRESULT) {
	    DeliveryResult_t result;
	    if(!ss -> ReceiveDeliveryResult(&result)) {
	        std::cout << "Error receiving delivery result" << std::endl;
            } else {
                std::cout << "Writing delivery result in xml" << std::endl;
                RdkCTHAddDeliveryJSONFile(result);
	    }
	    goto read_processed_data;
        }
        std::cout << "Error receiving metadata, received:" << htype <<std::endl;
        //return false;
    }

    //std::cout << "Object Count:" << msg.frameInfo.meta_data.objectCount << std::endl;
    //std::cout << "Event Count: " << msg.frameInfo.meta_data.eventCount << std::endl;

    //int ctr = 0;
    //while(ctr < msg.frameInfo.meta_data.objectCount){
    //    std::cout << "lcol:" << msg.frameInfo.meta_data.objects[ctr].lcol << std::endl;
    //    std::cout << "rcol:" << msg.frameInfo.meta_data.objects[ctr].rcol << std::endl; 
    //    std::cout << "trow:" << msg.frameInfo.meta_data.objects[ctr].trow << std::endl; 
    //    std::cout << "brow:" << msg.frameInfo.meta_data.objects[ctr].brow << std::endl; 
    //    std::cout << "object_class:" << msg.frameInfo.meta_data.objects[ctr].object_class << std::endl; 
    //    ctr++;
    //}

    //ctr = 0;
    //while(ctr < msg.frameInfo.meta_data.eventCount) {
    //    std::cout << "event_type:" << msg.frameInfo.meta_data.events[ctr].event_type;
    //    ctr++;
    //} 
    
    THPushMessage(msg);
    msg.frameInfo.image.release();

    if((clientType == TH_CAMERA) && (frameCountLimit == frameCtr)){
        firstFrame = true;
        frameCtr = 0;
    }

    return true;
}

/**
* @brief  This function is used to push the message to processing Thread
* @param  msg, message to posted for processing thread 
* @return void
*/
void THServer::THPushMessage(THMsgQueBuf msg)
{
    {
        std::unique_lock<std::mutex> lock(msgQueueMutex);
        msgQue.push(msg);
        lock.unlock();
    }
    msgCv.notify_one();
}

void THServer::ExitTHThread(const char *s)
{
    perror(s);
}

/**
 * @brief This function is used to reset all the objects count.
 * @param void.
 * @return 0 on success, -1 on failure.
 */
void THServer::RdkCTHResetObjEvtCount()
{
    /* Reset block */
    HDCount = 0;
    UDCount = 0;
    TDCount = 0;
    VDCount = 0;
    PDCount = 0;
    EventCount = 0;

}

/**
* @brief This function is used to process the frame for objects
* @param frame, frame to processed
* @return 0 on success, -1 on failure.
*/
int THServer::RdkCTHGenerateProcessedFrame(THFrameInfo frame)
{

    int temp = 0;
    int i = 0;
    int j = 0;
    int tempHDCount = 0;
    int tempUDCount = 0;
    int tempTDCount = 0;
    int tempVDCount = 0;
    int tempPDCount = 0;

    /* matrix to store color image */
    cv::Mat fileFrameBGR = cv::Mat( frame.image.size().height, frame.image.size().width, CV_8UC4);

    //Event 
    int x_cord = fileFrameBGR.size().width - 200;
    int y_cord = 20;
    int y_cord_time = fileFrameBGR.size().height - 40;
    cv::Point textOrgForEventCount(x_cord,y_cord);
    
    //memset(EventInfo, 0, EVENT_LENGTH);
    EventInfo = "";

    //Time
    cv::Point textOrgForTime(x_cord,y_cord_time);

    /* convert the frame to BGR format */
    fileFrameBGR =  frame.image;

    if( raw_video == true ) {
	if(outputProcessedVideo) {
	    g_videoWriter.write(fileFrameBGR);
        }
	return 0;
    }

    /* process the frame to mark all the object detected */
    int objectCounter = frame.meta_data.objectCount;
    int eventCounter = frame.meta_data.eventCount;

    while(objectCounter-- > 0) {
        cv::Point x( frame.meta_data.objects[i].lcol,frame.meta_data.objects[i].trow );
        cv::Point y( frame.meta_data.objects[i].rcol,frame.meta_data.objects[i].brow );

        cv::Rect rect( x, y );
        if (frame.meta_data.objects[i].object_class == th_eOC_Human) {
            /* Draw a rectangle on the detected human face */
            rectangle( fileFrameBGR, rect, boxColorForHuman, BOX_THICKNESS );
            HDCount++;
            tempHDCount++;
        } else if (frame.meta_data.objects[i].object_class == th_eOC_Vehicle) {
            /* Draw a rectangle on the detected object */
            rectangle( fileFrameBGR, rect, boxColorForVehicle, BOX_THICKNESS );
            VDCount++;
            tempVDCount++;
        } else if (frame.meta_data.objects[i].object_class == th_eOC_Train) {
            /* Draw a rectangle on the detected object */
            rectangle( fileFrameBGR, rect, boxColorForTrain, BOX_THICKNESS );
            TDCount++;
            tempTDCount++;
        } else if (frame.meta_data.objects[i].object_class == th_eOC_Pet) {
            /* Draw a rectangle on the detected object */
            rectangle( fileFrameBGR, rect, boxColorForPet, BOX_THICKNESS );
            PDCount++;
            tempPDCount++;
        } else {
            /* Draw a rectangle on the detected object */
            rectangle( fileFrameBGR, rect, boxColorForUnknown, BOX_THICKNESS );
            UDCount++;
            tempUDCount++;
        }
        i++;
    }

    /* count no. of objects */
    OBJString = "Total Objects:" + std::to_string(HDCount+UDCount+PDCount+TDCount+VDCount);
    HDString  = "Human:" + std::to_string(HDCount);
    VDString  = "Vehicle:" + std::to_string(VDCount);
    TDString  = "Train:" + std::to_string(TDCount);
    PDString  = "Pet:" + std::to_string(PDCount);
    UDString  = "Unknown:" + std::to_string(UDCount);

    putText(fileFrameBGR, "Events:", cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);

    if(tempHDCount > 0 ){
        EventCount++;
        temp++;
        putText(fileFrameBGR, "HUMAN DETECTED", cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
    }

#if 0
    if(tempUDCount > 0 ){
        EventCount++;
        temp++;
        putText(fileFrameBGR, "MOTION DETECTED", cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
    }
#endif

    if(tempVDCount > 0 ){
        EventCount++;
        temp++;
        putText(fileFrameBGR, "VEHICLE DETECTED", cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
    }
    if(tempTDCount > 0 ){
        EventCount++;
        temp++;
        putText(fileFrameBGR, "TRAIN DETECTED", cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
    }
    if(tempPDCount > 0 ){
        EventCount++;
        temp++;
        putText(fileFrameBGR, "PET DETECTED", cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
    }

    /* process the frame to mark all the event detected */
    while(eventCounter-- >0) {
        switch(frame.meta_data.events[j].event_type)
        {
            case th_eSceneChange:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "CAMERA TAMPERED");
                EventInfo = "CAMERA TAMPERED";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;

            case th_eLightsOn:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "LIGHTS ON");
                EventInfo = "LIGHTS ON";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;

            case th_eLightsOff:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "LIGHTS OFF");
                EventInfo = "LIGHTS OFF";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;

            case th_eMotionDetected:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "MOTION DETECTED");
                EventInfo = "MOTION DETECTED";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);

                if(frame.meta_data.isMotionInsideROI) {
                    temp++;
                    putText(fileFrameBGR, "Inside ROI", cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForROIMotion, TEXT_THICKNESS);
                }
                if(frame.meta_data.isMotionInsideDOI) {
                    temp++;
                    putText(fileFrameBGR, "Inside DOI", cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForDOIMotion, TEXT_THICKNESS);
                }


            break;

            case th_eObjectEnteredEvent:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "OBJECT ENTERED");
                EventInfo = "OBJECT ENTERED";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;

            case th_eObjectExitedEvent:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "OBJECT EXITED");
                EventInfo = "OBJECT EXITED";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;

            case th_eObjectLeftEvent:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "OBJECT LEFT");
                EventInfo = "OBJECT LEFT";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;

            case th_eObjectLost:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "OBJECT LOST");
                EventInfo = "OBJECT LOST";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;

            case th_eObjectRemovedEvent:
                EventCount++;
	            temp++;
                //sprintf(EventInfo, "%s", "OBJECT REMOVED");
                EventInfo = "OBJECT REMOVED";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;
            case th_eUnknownEvent:
                EventCount++;
                temp++;
                //sprintf(EventInfo, "%s", "UNKNOWN EVENT");
                EventInfo = "UNKNOWN EVENT";
                putText(fileFrameBGR, EventInfo, cv::Point(x_cord,temp*20+40), cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEvent, TEXT_THICKNESS);
            break;
        }
        j++;

    }

    /* No. of events */
    //sprintf(EventString, "Total Events: %d ", EventCount);
    EventString = "Total Events:" + std::to_string(EventCount);

    /* Provide all the information regarding objects and events on frame */
    putText(fileFrameBGR, OBJString, textOrgForObj, cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForObj, TEXT_THICKNESS);
    putText(fileFrameBGR, HDString, textOrgForHuman, cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForHuman, TEXT_THICKNESS);
    putText(fileFrameBGR, UDString, textOrgForVehicle, cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForUnknown, TEXT_THICKNESS);
    putText(fileFrameBGR, EventString, textOrgForEventCount, cv::FONT_HERSHEY_SIMPLEX, FONT_SCALE , textColorForEventCount, TEXT_THICKNESS);

    if(outputProcessedVideo) {
        g_videoWriter.write(fileFrameBGR);
    }

    if(supportFrameDisplay){
        cv::imshow(clientMAC, fileFrameBGR);
        if(cv::waitKey(30) >= 0) return 0;
    }

    return 0;
}

/** @brief VA Message Queue data is monitored in separate thread and following is the thread function.
 *  @param arg parameter for creating thread
 *  @return None
 */
void THServer::MessageQueueThreadFunc()
{
    THMsgQueBuf RecvBuf = {};
    bool exitProcessLoop = false;

    FILE *fileDesc = NULL;
    std::string videoInfoFilePath;
    std::string clientDir;
    std::string currVideoFile;
    std::string processedVideoFilename;
    std::string xmlFilename;
    std::string command;


    while(!exitProcessLoop) {
            {
                std::unique_lock<std::mutex> lock(msgQueueMutex);
                msgCv.wait(lock,[this]{return !msgQue.empty();});
                
                RecvBuf = msgQue.front();
	            msgQue.pop();

                lock.unlock();
            }

            switch( RecvBuf.msgType ) {

                /* Receiving the details of an application  */
                case TH_CLIENT_INFO:
                {
                    /* Create an folder named this mac id */
		            clientDir =  RecvBuf.clientInfo.outputDir + "/" + RecvBuf.clientInfo.mac;

                    command.clear();
                    command = "mkdir -p " + clientDir;
                    std::cout << "about to execute Command:" <<  command << std::endl;
                    system(command.c_str());

                    videoInfoFilePath = clientDir + "/videoInfo.txt";
                    fileDesc = fopen(videoInfoFilePath.c_str(), "a");

                    if(!fileDesc){
                        std::cout << "Error creating video info file .." << std::endl;
                        exitProcessLoop = true;
                    }

                    break;
                }
                case TH_FILE_INFO:
                {
                    second=0, minute =0, hour=0, temp_frame_count = 0;

                    currVideoFile = RecvBuf.fileInfo.name;

                    processedVideoFilename =  clientDir + "/Processed_video_" + currVideoFile + ".avi";
                    xmlFilename = clientDir + "/Processed_json_" + RecvBuf.fileInfo.name + ".xml";

		            v_file_fps = RecvBuf.fileInfo.fps;
                
                    /* Frames are written in MJPEG format */
		    if(outputProcessedVideo) {
                        if(encodingFormat.compare("h264") == 0) {
                            g_videoWriter.open(processedVideoFilename.c_str(), CV_FOURCC('X','2','6','4'), 
                            RecvBuf.fileInfo.fps, cv::Size(RecvBuf.fileInfo.cols,RecvBuf.fileInfo.rows), true);
			} else if (encodingFormat.compare("hfyu") == 0) {
			    g_videoWriter.open(processedVideoFilename.c_str(), CV_FOURCC('H','F','Y','U'),
			    RecvBuf.fileInfo.fps, cv::Size(RecvBuf.fileInfo.cols,RecvBuf.fileInfo.rows), true);
                        } else {
                            g_videoWriter.open(processedVideoFilename.c_str(), CV_FOURCC('M','J','P','G'), 
                            RecvBuf.fileInfo.fps, cv::Size(RecvBuf.fileInfo.cols,RecvBuf.fileInfo.rows), true);
			}
                    }

                    //Time stamp
                    time (&file_start_time);
                    timeinfo = localtime (&file_start_time);
                    /* Create a JSON file */
                    RdkCTHPrepareJSONHeader(xmlFilename.c_str(), timeinfo);

		    if(outputProcessedVideo) {
                        if(!g_videoWriter.isOpened())
                        {
                            std::cout << "Error while opening file" << std::endl;
                            ExitTHThread("g_videoWriter.open");
                        }
		    }

                    frameNum = RecvBuf.fileInfo.frameTotal;

		            RdkCTHResetObjEvtCount();

                    break;
                }
                /* For sending motion detected event to the application  */
                case TH_FRAME_INFO:
                {
                    RdkCTHGenerateProcessedFrame(RecvBuf.frameInfo);

                    if ((RecvBuf.frameInfo.meta_data.objectCount != 0) || (RecvBuf.frameInfo.meta_data.eventCount != 0))
                        RdkCTHAddJSONInfo(RecvBuf.frameInfo);


                    if(frameNum > RecvBuf.frameInfo.frameIndex)
                        std::cout << "MAC:" << clientMAC << "Processed frame " << RecvBuf.frameInfo.frameIndex << " of " << frameNum << ", For " << currVideoFile << std::endl;

                    break;
                }

                case TH_FILE_END:
                {
//                    else if (frameNum == RecvBuf.frameInfo.frameIndex){
                    if( !currVideoFile.empty() ) {

                        RdkCTHWriteJSONInfoToFile(xmlFilename.c_str());
                        std::cout << "Processing complete for " << currVideoFile << std::endl;

                        fwrite(currVideoFile.c_str(), sizeof(char), currVideoFile.length(), fileDesc);
                        fwrite("\n", sizeof(char), 1, fileDesc);
                        fflush(fileDesc);
                    }
                    break;
                }

                case TH_PROCESS_THREAD_EXIT:
                {   
                    fclose(fileDesc);
                    std::cout << "Exiting Process Loop" << std::endl;
                    exitProcessLoop = true;
                    break;
                }

                default:
                    //RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","%s(%d): Invalid message type.\n", __FUNCTION__, __LINE__);
                    break;
            }
	    //sleep(5);
    }
}

/**@brief This function is used to generate the current timestamp
*  @param timestamp, current timestamp
*  @return void
*/
void THServer::GetTimeStamp(char* timestamp)
{
#if 0
    temp_frame_count++;
    second = (int(temp_frame_count/v_file_fps));
    if(second >= 60) {
        second = 0;
        temp_frame_count = 0;
        minute++;
        if(minute >= 60) {
            minute = 0; hour++;
        }
    }
    sprintf(timestamp,"%d:%d:%d",hour,minute,second);
#else
    temp_frame_count++;
    minute = 0;
    hour = 0;
    second = (int)(temp_frame_count/v_file_fps);
    if(second >= 60) {
        minute = second / 60;
        second = second % 60;
        //temp_frame_count = 0;
        if(minute >= 60) {
            minute = minute % 60;
            hour = minute / 60;
        }
    }
    sprintf(timestamp,"%d:%d:%d",hour,minute,second);
#endif
}

/**
 * @brief This function is used to form json initial header
 * @param xml_file_name , xml file name
 * @param timeinfo , time the first frame is received
 * @return , None
 */
void THServer::RdkCTHPrepareJSONHeader(const char * xml_filename ,struct tm * timeinfo)
{

        char time_str[STR_LENGTH] = {0};
        cJSON * json_file_obj;
        g_json_message = cJSON_CreateObject();

        json_file_obj = cJSON_CreateString(xml_filename);
        if (( NULL != g_json_message ) && ( NULL != json_file_obj ))
        {
                //RDK_LOG( RDK_LOG_INFO,"before adding json infor:%s:%s:%u: \n", json_file_obj->valuestring, json_file_obj->string , json_file_obj->type );

                cJSON_AddItemToObject( g_json_message, JSON_FILENAME , json_file_obj);

                //RDK_LOG( RDK_LOG_INFO,"before adding json infor:%s:%s:%u: \n", json_file_obj->valuestring, json_file_obj->string , json_file_obj->type );
                strftime (time_str , STR_LENGTH ,"%m/%d/%Y: %H:%M:%S:",timeinfo);

                cJSON_AddStringToObject( g_json_message , JSON_FILE_TIMESTAMP , time_str);


                g_object_event_array = cJSON_CreateArray();
                g_delivery_result_array = cJSON_CreateArray();
        } else
        {
             //RDK_LOG( RDK_LOG_ERROR,"could not allocate json object:%s:%d:\n",__FUNCTION__,__LINE__);
        }
}

int THServer::getFileName(int fnum, char * fname)
{
    if(processedFileList.size() < fnum) {
        std::cout << "Requested file index(" << fnum << ") is greater than the list size(" << processedFileList.size() << ")" << std::endl;
        return -1;
    }

    strcpy(fname, processedFileList.at(fnum - 1).c_str());
    return 0; 
}

void THServer::RdkCTHAddDeliveryJSONFile(DeliveryResult_t result)
{

    char fileName[STR_LENGTH];
    if(getFileName(result.fileNum, fileName) == -1) {
        std::cout << "Could not find file name, skipping the delivery result" << std::endl;
	return;
    }
    cJSON * combined_json_object = cJSON_CreateObject();
    cJSON * person_json_array = cJSON_CreateArray();
    cJSON * json_fileName = cJSON_CreateString(fileName);

    if ( ( NULL != person_json_array) && ( NULL != combined_json_object) && ( NULL != g_delivery_result_array )) {

        //cJSON_AddItemToObject(combined_json_object, TIME_STAMP  , json_frame_time_stamp);
        cJSON_AddItemToObject(combined_json_object, "File"  , json_fileName);
	cJSON_AddNumberToObject(combined_json_object, "frameNumber", result.frameNum);
	cJSON_AddNumberToObject(combined_json_object, "deliveryScore", result.deliveryScore);
	cJSON_AddNumberToObject(combined_json_object, "maxAugScore", result.maxAugScore);
	cJSON_AddNumberToObject(combined_json_object, "motionTriggeredTime", result.motionTriggeredTime);
	cJSON_AddNumberToObject(combined_json_object, "mpipeProcessedframes", result.mpipeProcessedframes);
	cJSON_AddNumberToObject(combined_json_object, "time_taken", result.time_taken);
	cJSON_AddNumberToObject(combined_json_object, "time_waited", result.time_waited);
        cJSON_AddNumberToObject(combined_json_object, "personCount", result.personScores.size());
        cJSON_AddItemToObject(combined_json_object, "personInfo"  , person_json_array);
	for(int i = 0; i < result.personScores.size(); i++) {

            cJSON * person_json_info = cJSON_CreateObject();

	    if( NULL != person_json_info ) {
                cJSON_AddItemToArray(person_json_array , person_json_info);
                cJSON_AddNumberToObject(person_json_info, "xCord", result.personBBoxes[i][0]);
                cJSON_AddNumberToObject(person_json_info, "yCord", result.personBBoxes[i][1]);
                cJSON_AddNumberToObject(person_json_info, "width", result.personBBoxes[i][2]);
                cJSON_AddNumberToObject(person_json_info, "height", result.personBBoxes[i][3]);
                cJSON_AddNumberToObject(person_json_info, "personScore", result.personScores[i]);
                if(server_version > TH_VERSION_1_2) {
                    cJSON * json_true = cJSON_CreateString("true");
                    cJSON_AddItemToObject(person_json_info, "insideROI", json_true);
                }
            }
	    
	}
        if(server_version > TH_VERSION_1_2) {
	    for(int i = 0; i < result.nonROIPersonScores.size(); i++) {

                cJSON * person_json_info = cJSON_CreateObject();

                if( NULL != person_json_info ) {
                    cJSON_AddItemToArray(person_json_array , person_json_info);
                    cJSON_AddNumberToObject(person_json_info, "xCord", result.nonROIPersonBBoxes[i][0]);
                    cJSON_AddNumberToObject(person_json_info, "yCord", result.nonROIPersonBBoxes[i][1]);
                    cJSON_AddNumberToObject(person_json_info, "width", result.nonROIPersonBBoxes[i][2]);
                    cJSON_AddNumberToObject(person_json_info, "height", result.nonROIPersonBBoxes[i][3]);
                    cJSON_AddNumberToObject(person_json_info, "personScore", result.nonROIPersonScores[i]);
                    cJSON * json_false = cJSON_CreateString("false");
                    cJSON_AddItemToObject(person_json_info, "insideROI", json_false);
                }
	    }
	}

        cJSON_AddItemToArray(g_delivery_result_array, combined_json_object);
#if 0
        char * str = cJSON_Print(combined_json_object);
        cJSON_Delete(combined_json_object);
        std::string fullpath = outputVideoDir + "/" + clientMAC + "/" + DD_RESULT_FILE;
        FILE * f_xml = fopen(fullpath.c_str(), "a+");
        if ( (NULL != f_xml) && ( NULL != str))
        {
            int rc = fwrite(str , 1 , strlen(str),f_xml);
            if ( EOF == rc )
            {
                //RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","could not write to file:%s:\n",xml_filename);
                //printf( " xml file fputs error ,%s rc:%d:line:%d:\n",xml_filename,rc,__LINE__);
                rc = fclose(f_xml);
            } else
            {
                rc = fclose(f_xml);
                sync();
            }
            free(str);
        }
#endif
    }
}
/**
 * @brief This function is used to add object , event information for each frame in json format
 * @param frame, frame information (objects and event detected) 
 * @return , None
 */
void THServer::RdkCTHAddJSONInfo(THFrameInfo frame)
{
    char time_str[STR_LENGTH] = {0};
    int i;

    int _objectCounter = frame.meta_data.objectCount;
    int _eventCounter = frame.meta_data.eventCount;

    GetTimeStamp(&time_str[0]);
    
    cJSON * object_json_array = cJSON_CreateArray();
    cJSON * event_json_array = cJSON_CreateArray();
    cJSON * combined_json_object = cJSON_CreateObject();
    cJSON * json_frame_time_stamp = cJSON_CreateString(time_str);
    cJSON * json_roi_motion = cJSON_CreateString( (frame.meta_data.isMotionInsideROI == true) ? "true" : "false" );
    cJSON * json_doi_motion = cJSON_CreateString( (frame.meta_data.isMotionInsideDOI == true) ? "true" : "false" );

    if ( ( NULL != object_json_array) && ( NULL != event_json_array ) &&
        ( NULL != combined_json_object) && ( NULL != g_object_event_array )) {

        cJSON_AddNumberToObject(combined_json_object, FRAME_COUNTER, frame.frameIndex);
        cJSON_AddItemToObject(combined_json_object, OBJECT_INFO  , object_json_array);
        cJSON_AddItemToObject(combined_json_object, EVENT_INFO  , event_json_array );
        cJSON_AddItemToObject(combined_json_object, TIME_STAMP  , json_frame_time_stamp);
        cJSON_AddItemToObject(combined_json_object, ROI_MOTION , json_roi_motion);
        cJSON_AddItemToObject(combined_json_object, DOI_MOTION , json_doi_motion);

        i=0;
        while(_objectCounter-- > 0) {

            cJSON * object_json_info = cJSON_CreateObject();
            if ( NULL != object_json_info ) {
                cJSON_AddItemToArray( object_json_array , object_json_info );
                cJSON_AddNumberToObject(object_json_info, OBJECT_M_E_CLASS , frame.meta_data.objects[i].object_class);
                cJSON_AddNumberToObject(object_json_info, OBJECT_M_I_LCOL, frame.meta_data.objects[i].lcol);
                cJSON_AddNumberToObject(object_json_info, OBJECT_M_I_RCOL, frame.meta_data.objects[i].rcol);
                cJSON_AddNumberToObject(object_json_info, OBJECT_M_I_TROW, frame.meta_data.objects[i].trow);
                cJSON_AddNumberToObject(object_json_info, OBJECT_M_I_BROW, frame.meta_data.objects[i].brow);
            } else {
                //RDK_LOG( RDK_LOG_ERROR,"could not allocate json object:%s:%d:\n",__FUNCTION__,__LINE__);
            }

            i++;
        }

        i=0;

        while(_eventCounter-- >0) {
            cJSON * event_json_info = cJSON_CreateObject();
            if ( NULL != event_json_info ) {
                cJSON_AddItemToArray( event_json_array , event_json_info);
                cJSON_AddNumberToObject(event_json_info  , EVENT_M_E_TYPE , frame.meta_data.events[i].event_type);
            } else {
                //RDK_LOG( RDK_LOG_ERROR,"could not allocate json object:%s:%d:\n",__FUNCTION__,__LINE__);
            }
            i++ ;
        }
        cJSON_AddItemToArray( g_object_event_array , combined_json_object  );
    } else {
        //RDK_LOG( RDK_LOG_ERROR,"could not allocate json object:%s:%d:\n",__FUNCTION__,__LINE__);
    }
}

/**
 * @brief This function is used to add json information to a file
 * @return 0 on success, otherwise -1
 */
int THServer::RdkCTHWriteJSONInfoToFile(const char * xml_filename)
{
        char *str = NULL;
        FILE *f_xml = NULL ;
        int rc = 0;
        int ret = -1 ;
        char time_str[STR_LENGTH] = {0};


        if ( ( NULL == g_json_message ) || ( NULL == g_object_event_array ) || ( NULL == g_delivery_result_array ))
        {
                //RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","json object is NULL:%s:%d:\n",__FUNCTION__,__LINE__);
                std::cout << "json object is null" << std::endl;
                ret = -1;
        } else
        {
                //Time stamp
                time (&timestamp);
                timeinfo = localtime (&timestamp);
                strftime (time_str , STR_LENGTH ,"%m/%d/%Y: %H:%M:%S:",timeinfo);
                cJSON_AddStringToObject( g_json_message, JSON_FILE_END_TIMESTAMP , time_str);

                double time_diff = difftime(file_start_time, timestamp);
		memset(time_str, 0, sizeof(time_str));
		sprintf(time_str, "%dm %ds", time_diff/60, (int)fmod(time_diff, 60));
                printf("Time taken to process: %s", time_str);

                cJSON_AddStringToObject( g_json_message, JSON_FILE_PROCESS_TIME, time_str);
                cJSON_AddItemToObject(g_json_message, OBJECT_EVENT_INFO , g_object_event_array );
                cJSON_AddItemToObject(g_json_message, DELIVERY_RESULTS_INFO , g_delivery_result_array );
                str = cJSON_Print(g_json_message);
                cJSON_Delete(g_json_message);
                f_xml = fopen(xml_filename ,"w+");
                if ( (NULL != f_xml) && ( NULL != str))
                {
                        rc = fwrite(str , 1 , strlen(str),f_xml);
                        if ( EOF == rc )
                        {
                                //RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","could not write to file:%s:\n",xml_filename);
                                printf( " xml file fputs error ,%s rc:%d:line:%d:\n",xml_filename,rc,__LINE__);
                                rc = fclose(f_xml);
                                ret = -1;
                        } else
                        {
                                rc = fclose(f_xml);
                                sync();
                        }
                        free(str);
                } else
                {
                        //RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.TESTHARNESS","could not open file:%s: errornum:%d:\n",xml_filename,errno);
                        ret = -1;
                }
        }
        return ret;
}

/**
* @brief This function is used to read configuration files
* @return , bool, true on sucess, else false.
*/
bool THServer::loadSourceFileConfig(std::string fname)
{
    std::ifstream in(fname.c_str());

    memset(&config, 0, sizeof(config));

    if (!in.is_open()) {
        std::cout << "Error: Unable to open settings file \"" << fname << "\" for reading!" << std::endl;
        return false;
    }

    std::string line;
    const std::locale m_locale;
    while (std::getline(in, line)) {
        if (line.size() > 0 && line[0] != '#') {
            size_t index = 0;
            // trim leading whitespace
            while (std::isspace(line[index], m_locale))
                index++;
            // get the key string
            const size_t beginKeyString = index;
            while (!std::isspace(line[index], m_locale) && line[index] != '=')
                index++;
            const std::string key = line.substr(beginKeyString, index - beginKeyString);

            // skip the assignment
            while (std::isspace(line[index], m_locale) || line[index] == '=')
                index++;

            // get the value string
            const std::string value = line.substr(index, line.size() - index);

            if(key.compare(std::string("roi")) == 0) {
		config.roiCoords = value;
            } else if(key.compare(std::string("doi_threshold")) == 0) {
		config.doi_threshold = std::stoi(value);
            } else if(key.compare(std::string("doi_bitmap")) == 0) {
		config.doi_bitmap = value;
            }
        }
    }

    in.close();
    return true;
}

/**
* @brief This function is used to monitor any new files added to source directory 
* @return , None
*/
void THServer::THMonitorVideoFile( ) 
{
    int evtCount, ctr;
    int fd;
    int wd;
    int activity = -1;
    char buffer[BUF_LEN];

    fd_set readfds;
    struct timeval time_to_wait;
 
    fd = inotify_init();
 
    if ( fd < 0 ) {
        std::cerr << "inotify_init" << std::endl;
    }
 
    wd = inotify_add_watch( fd, srcVideoPath.c_str(), IN_CLOSE_WRITE);

    while(true){

        ctr = 0;

        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        time_to_wait.tv_sec = 3;
        time_to_wait.tv_usec = 0;

        activity = select ( fd+1, &readfds, NULL, NULL, &time_to_wait);
        if(activity < -1)
            std::cerr << "select" << std::endl;

        else if((activity > 0) && (FD_ISSET(fd, &readfds)))  {

            evtCount = read( fd, buffer, BUF_LEN );  
 
            if ( evtCount < 0 ) {
                std::cerr << "read" << std::endl;
            }
 
            while ( ctr < evtCount) {
                struct inotify_event *event = ( struct inotify_event * ) &buffer[ ctr ];
                if ( event->len ) {
                    if ( event->mask & IN_CLOSE_WRITE ) {
                        if ( event->mask & IN_ISDIR ) {
                            std::cout << event->name << "is a directory, cannot Process a Directory!!" << std::endl;
                        }
                        else if (strstr(event->name, CONF_FILE_SUFFIX) == NULL) {
                            std::unique_lock<std::mutex> lock(videoListMutex);
                            videoList.push(event->name);
                            std::cout << "New file detected" << std::endl;
                            lock.unlock();
                            vListCv.notify_one();
                        }
                    }
                    /* else if ( event->mask & IN_DELETE ) {
                        if ( event->mask & IN_ISDIR ) {
                            printf( "The directory %s was deleted.\n", event->name );
                        }
                        else {
                            printf( "The file %s was deleted.\n", event->name );
                        }
                    }
                    else if ( event->mask & IN_MODIFY ) {
                        if ( event->mask & IN_ISDIR ) {
                            printf( "The directory %s was modified.\n", event->name );
                        }
                        else {
                            printf( "The file %s was modified.\n", event->name );
                        }
                    } */
                }
                ctr += EVENT_SIZE + event->len;
            }
        }
    }
 
    ( void ) inotify_rm_watch( fd, wd );
    ( void ) close( fd );
}

void THServer::THServerStop()
{
    if(ss){
        delete ss;
    }

    if(thServerInstance){
        delete thServerInstance;
    }
}

THServer::~THServer()
{
    system("killall THServer");
}
