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

#ifndef TH_SERVER_HPP
#define TH_SERVER_HPP

#include <iostream>
#include <fstream>
#include <queue>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"

/*      JSON Include    */
#include "cJSON/cJSON.h"
#include <errno.h>

#include "socket_server.hpp"
#include "TestHarnessMsgQueue.h"

typedef struct
{
    std::string roiCoords;
    int doi_threshold;
    std::string doi_bitmap;
} SourceFileConfig;


class THServer
{
public:
    //member functions
    bool THServerInit(std::string configFilePath);
    bool THServerStart();
    void THServerStop();

    bool THProcessFrames();

    void THPushMessage(THMsgQueBuf msg);
    static THServer* THCreateServerInstance();

private:

    THServer();
    ~THServer();
    bool ReceiveTHInfo();
    Status SendFrame();
    int RdkCTHGenerateProcessedFrame(THFrameInfo frame);
    void MessageQueueThreadFunc();
    void THMonitorVideoFile();
    void RdkCTHPrepareJSONHeader(const char * xml_filename ,struct tm * timeinfo);
    void RdkCTHAddJSONInfo(THFrameInfo frame);
    void RdkCTHAddDeliveryJSONFile(DeliveryResult_t result);
    void RdkCTHAddUploadStatusJSONFile(smtnUploadStatus_t stnStatus);
    int getFileName(int, char*);
    int RdkCTHWriteJSONInfoToFile(const char * xml_filename);
    void ExitTHThread(const char *s);
    void RdkCTHResetObjEvtCount();
    void GetTimeStamp(char* timestamp);
    bool THVideoListIsEmpty();
    Status THPopulateVideoList();
    void THGetProcessedVideos();
    std::string THTrim(const std::string& str);
    void THArchiveProcessedFiles();
    bool loadSourceFileConfig(std::string fname);

    //static variables
    static THServer *thServerInstance;

    //member variables
    int port;
    SocketServer *ss;
    Client clientType;
    enum ServerVersion server_version;
    SourceFileConfig config;
    std::string clientMAC;
    std::string clientProcessedVideoDir;
    std::string outputVideoDir;
    std::string encodingFormat;

    THMsgQueBuf msg;
    std::queue<THMsgQueBuf> msgQue;

    std::string srcVideoPath;
    DIR *sourceVideoDir;
    dirent *videoFile;
    std::queue<std::string> videoList;
    std::set<std::string> recurringVideos;
    std::vector<std::string> processedFileList;

    cv::VideoCapture vCap;
    cv::Mat currFrame;
    cv::Mat nextFrame;
    unsigned long frameCtr;
    unsigned long frameNum;
    int fileNum;
    unsigned long frameCountLimit;
         
    bool processingDataInProgress;
    bool fileChangeStatus;
    bool firstFrame;

    bool supportFrameDisplay;
    bool resizeFrame;
    bool outputProcessedVideo;
    bool raw_video;
    bool archive_processed;
    int32_t segmentation_size;
    bool isRoiEnabled;
    bool isDoiEnabled;

    int ImageRows;
    int ImageCols;

    std::thread frameProcessThread;
    std::thread videoFileMonitor;
    std::mutex msgQueueMutex;
    std::mutex videoListMutex;
    std::condition_variable msgCv;
    std::condition_variable vListCv;

    std::string filepath;
    std::string OBJString;
    std::string HDString;
    std::string VDString;
    std::string TDString;
    std::string PDString;
    std::string UDString;

    int HDCount;
    int UDCount;
    int TDCount;
    int VDCount;
    int PDCount;

    double videoFPS;

    std::string EventString;
    std::string EventInfo;
    int EventCount;

    cv::VideoWriter g_videoWriter;
    cv::VideoCapture g_videoCapturer;

    time_t timestamp;
    time_t file_start_time;
    struct tm * timeinfo;
    int second;
    int minute;
    int hour;
    int temp_frame_count;

    double v_file_fps;

    int ResizedImageColumn;
    int ResizedImageRow;

    cJSON *g_json_message;
    cJSON *g_object_event_array;
    cJSON *g_delivery_result_array;
    cJSON *g_Upload_Status_array;
    static  cJSON * json_stnStatus;
    static int stnStatus_count;

};

#endif  //VA_SERVER_HPP
