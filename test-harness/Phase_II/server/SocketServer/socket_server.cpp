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

#include "socket_server.hpp"
#include <unistd.h> // close
#include <sys/types.h>
#include <sys/stat.h>
#include <opencv2/imgcodecs.hpp>
#include <iostream>

// ------------------------------------------------------------------------------------------------------------------
SocketServer::SocketServer(int port) :
        image_dims_(cv::Size2i(0, 0)),
        client_len_(0),
        server_addr_size_(sizeof(server_addr_)),
        port_(port),
        sock_fdesc_init_(0),
        sock_fdesc_conn_(0),
        fnum(0)  {
        client_len_ = server_addr_size_;
}
// ------------------------------------------------------------------------------------------------------------------
SocketServer::~SocketServer()
{
    //Close();
}
// ------------------------------------------------------------------------------------------------------------------

void SocketServer::Close() {

    shutdown(sock_fdesc_init_,SHUT_WR);

    if (sock_fdesc_init_ > 0)
    {
        close(sock_fdesc_init_);
    }
}
// ------------------------------------------------------------------------------------------------------------------
bool SocketServer::Connect() {

    int opt = 1;

    // Initialize Socket
    sock_fdesc_init_ = socket(AF_INET6, SOCK_STREAM, 0);

    if (sock_fdesc_init_ == -1) {
        close(sock_fdesc_init_);
        perror("Couldn't create socket!\n");
        return false;
    }

    // Zero out server address struct
    memset((char *) &server_addr_, 0, server_addr_size_);

    if (setsockopt(sock_fdesc_init_, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        return false;
    }

    // Set server address struct
    server_addr_.sin6_family = AF_INET6;
    server_addr_.sin6_addr = in6addr_any;
    server_addr_.sin6_port = htons(port_);

    // Assign server address to initial socket file descriptor
    if (bind(sock_fdesc_init_, (struct sockaddr *) &server_addr_, server_addr_size_) == -1) {
        perror("Couldn't bind initial socket file descriptor!");
        printf("Trying again after killing existing process on port %d...\n",
               port_);
        close(sock_fdesc_init_);
        if (bind(sock_fdesc_init_, (struct sockaddr *) &server_addr_,
                 server_addr_size_) == -1) {
            perror("Couldn't bind initial socket file descriptor after retry!");
            return false;
        }
        printf("Successful bind to port %d after killing previous process\n",
               port_);
    }

    // Enable listening on initial socket file descriptor
    listen(sock_fdesc_init_, 5);

    return true;
}

// ------------------------------------------------------------------------------------------------------------------
bool SocketServer::MonitorRequest() {

    while(true){

        pid_t pid = -1;

        // Block process until connection with client has been established.
        // 'client_fdesc' set as new file descriptor to be used for communication
	std::cout << "Monitoring the SocketServer Waiting for Client to join" << std::endl;
        sock_fdesc_conn_ = accept(sock_fdesc_init_, (struct sockaddr *) &client_addr_, &client_len_);

        if (sock_fdesc_conn_ == -1) {
            perror("ERROR! Client couldn't connect!");
            return false;
        }
	std::cout << "Creating a child process & exiting from Monitor Thread Loop" << std::endl;

        pid = fork();

        if (pid == 0)
            return true;
    }
}

// ------------------------------------------------------------------------------------------------------------------
HeaderType SocketServer::ReceiveHeader() {

    ssize_t bytes_recv = 0;
    size_t dims_size = 0;

    int cols = 0;
    int rows = 0;
    int header_type = H_WAITING;
   
    usleep(10000);
    if ( (bytes_recv = recv(sock_fdesc_conn_, (char *) &header_type, sizeof(header_type), 0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)header_type \n", __FUNCTION__, __LINE__);
        return H_ERROR;
    }
    if (header_type != (int) H_IMAGE)
        return (HeaderType) header_type;

    if ((bytes_recv = recv(sock_fdesc_conn_, (char *) &rows, sizeof(rows), 0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)rows \n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "image_size: %zu\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_, dims_size, bytes_recv);
        return H_ERROR;
    }


    if ((bytes_recv = recv(sock_fdesc_conn_, (char *) &cols, sizeof(cols), 0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)cols \n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "image_size: %zu\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_, dims_size, bytes_recv);
        return H_ERROR;
    }


    if ((bytes_recv = recv(sock_fdesc_conn_, (char *) &mat_type_, sizeof(mat_type_), 0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)mat_type \n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "image_size: %zu\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_, dims_size, bytes_recv);
        return H_ERROR;
    }

    image_dims_ = cv::Size2i(cols, rows);
    printf("Image dimensions: [%dx%d,%d]\n", cols, rows, mat_type_);

    return (HeaderType)header_type;
}
// ------------------------------------------------------------------------------------------------------------------
bool SocketServer::SendHeader(HeaderType type)
{
    int bytes_sent = 0;

    // Send number of rows to server
    if ((bytes_sent = send(sock_fdesc_conn_, (char*)&type, sizeof(type), 0)) == -1) {
        printf("ERROR!: send failed: (%s:%d)type \n", __FUNCTION__, __LINE__);
        perror("Error sending data");
        return false;
    }
    return true;
}
// ------------------------------------------------------------------------------------------------------------------
ServerVersion SocketServer::ReceiveServerVersion()
{
    ssize_t bytes_recv = 0;

    if((bytes_recv=recv(sock_fdesc_conn_, (char*) &serverVersion, sizeof(int32_t),0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)serverVersion \n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
    }

    return serverVersion;
//return 3;
}
//-------------------------------------------------------------------------------------------------------------------
Client SocketServer::ReceiveClientType()
{
    ssize_t bytes_recv = 0;
    int  client_type = TH_UNKNOWN;

    if((bytes_recv=recv(sock_fdesc_conn_, (char*) &client_type, sizeof(int),0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)client_type\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
    }

    return (Client)client_type;
}
//--------------------------------------------------------------------------------------------------------------------
bool SocketServer::ReceiveStnUploadStatus(smtnUploadStatus_t * stnStatus )
{
    if(recv(sock_fdesc_conn_, (char*) &(stnStatus->status), sizeof(stnStatus->status),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: fileNum\n");
        return false;
    }
    return true;
}
// ------------------------------------------------------------------------------------------------------------------
bool SocketServer::ReceiveDeliveryResult(DeliveryResult_t* result) {


/*    if(recv(sock_fdesc_conn_, (char*) &(result->timestamp), sizeof(result->timestamp),0) == -1) {
        printf("ERROR!: recv failed: timestamp\n");
        return false;
    }*/

    if(recv(sock_fdesc_conn_, (char*) &(result->fileNum), sizeof(result->fileNum),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: fileNum\n");
        return false;
    }

    if(recv(sock_fdesc_conn_, (char*) &(result->frameNum), sizeof(result->frameNum),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: frameNum\n");
        return false;
    }

    int count = 0;
    if(recv(sock_fdesc_conn_, (char*) &count, sizeof(int),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: count\n");
        return false;
    }

    for(int i = 0; i < count; i++) {
        std::vector<int> bbox;
        for(int j = 0; j < 4; j++) {
            int coord = 0;
            if(recv(sock_fdesc_conn_, (char*) &coord, sizeof(coord),0) == -1) {
                printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
                printf("ERROR!: recv failed: coords\n");
                return false;
            }
            bbox.push_back(coord);
        }
	result->personBBoxes.push_back(bbox);

	double score = 0;
	if(recv(sock_fdesc_conn_, (char*) &score, sizeof(score),0) == -1) {
            printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
            printf("ERROR!: recv failed: person score\n");
            return false;
        }
        result->personScores.push_back(score);
    }

    if(serverVersion > TH_VERSION_1_0) {
        count = 0;
        if(recv(sock_fdesc_conn_, (char*) &count, sizeof(int),0) == -1) {
            printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
            printf("ERROR!: recv failed: count\n");
            return false;
        }

        for(int i = 0; i < count; i++) {
            std::vector<int> bbox;
            for(int j = 0; j < 4; j++) {
                int coord = 0;
                if(recv(sock_fdesc_conn_, (char*) &coord, sizeof(coord),0) == -1) {
                    printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
                    printf("ERROR!: recv failed: coords\n");
                    return false;
                }
                bbox.push_back(coord);
            }
	    result->nonROIPersonBBoxes.push_back(bbox);

	    double score = 0;
	    if(recv(sock_fdesc_conn_, (char*) &score, sizeof(score),0) == -1) {
                printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
                printf("ERROR!: recv failed: person score\n");
                return false;
            }
            result->nonROIPersonScores.push_back(score);
        }
    }

    if(recv(sock_fdesc_conn_, (char*) &(result->deliveryScore), sizeof(result->deliveryScore),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: deliveryScore\n");
        return false;
    }

    if(recv(sock_fdesc_conn_, (char*) &(result->maxAugScore), sizeof(result->maxAugScore),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: maxAugScore\n");
        return false;
    }

    if(recv(sock_fdesc_conn_, (char*) &(result->motionTriggeredTime), sizeof(result->motionTriggeredTime),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: motionTriggeredTime\n");
        return false;
    }

    if(recv(sock_fdesc_conn_, (char*) &(result->mpipeProcessedframes), sizeof(result->mpipeProcessedframes),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: mpipeProcessedframes\n");
        return false;
    }

    if(recv(sock_fdesc_conn_, (char*) &(result->time_taken), sizeof(result->time_taken),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: time_taken\n");
        return false;
    }

    if(recv(sock_fdesc_conn_, (char*) &(result->time_waited), sizeof(result->time_waited),0) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed: time_waited\n");
        return false;
    }

    return true;
}

HeaderType SocketServer::ReceiveImage(cv::Mat &image) {


    HeaderType header_type = ReceiveHeader();

    if (header_type != H_IMAGE)
        return (HeaderType) header_type;

    ssize_t bytes_recv = 0;
    size_t image_size = 0;

    // Reset image
    image = cv::Mat::zeros(image_dims_, mat_type_);

    // Get image size
    image_size = image.total() * image.elemSize();

    // Allocate space for image buffer
    uchar *sock_data = image.ptr<uchar>();

    // Save image data to buffer
    for (int i = 0; i < image_size; i += bytes_recv) {

        bytes_recv = recv(sock_fdesc_conn_, sock_data + i, image_size - i, 0);

        if (bytes_recv == -1) {
            printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
            printf("ERROR!: recv failed\n"
                           "i: %d\n"
                           "sock_fdesc: %d\n"
                           "image_size: %d\n"
                           "bytes_recv: %d\n", i, sock_fdesc_conn_, (int)image_size, (int)bytes_recv);
            return H_ERROR;
        }
    }

    return header_type;
}
//------------------------------------------------------------------------------------------------------------------
bool SocketServer::SendDOI(int threshold, std::string bitmap) {
    int header_type = (int)H_DOI;

    int bytes_sent = 0;

    cv::Mat bitmap_image = cv::imread(bitmap.c_str(), cv::IMREAD_COLOR);
    if(bitmap_image.empty()) {
        printf("ERROR!: imead failed:DOI image (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error imread DOI bitmap");
        return false;
    }

    if (send(sock_fdesc_conn_, (char*)&header_type, sizeof(header_type), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending header");
        return false;
    }

    if (send(sock_fdesc_conn_, (char*)&threshold, sizeof(threshold), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending doi threshold");
        return false;
    }

    // Send number of rows to server
    if (send(sock_fdesc_conn_, (char*)&bitmap_image.rows, sizeof(bitmap_image.rows), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending rows");
        return false;
    }

    // Send number of cols to server
    if (send(sock_fdesc_conn_, (char*)&bitmap_image.cols, sizeof(bitmap_image.cols), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending cols");
        return false;
    }

    int matType = bitmap_image.type();// & CV_MAT_DEPTH_MASK;
    // Send number of cols to server
    if (send(sock_fdesc_conn_, (char*)&matType, sizeof(matType), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending type");
        return false;
    }

    cv::Mat out = bitmap_image;
    int image_size = bitmap_image.total() * bitmap_image.elemSize();

    if (bitmap_image.isContinuous() == false)
    {
        bitmap_image.copyTo(out);
    }

    bytes_sent = send(sock_fdesc_conn_, out.data, image_size, 0);

    if (bytes_sent != image_size) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending image");
        return false;
    }

    return true;
}

// ------------------------------------------------------------------------------------------------------------------
bool SocketServer::SendROI(std::string roi) {

    int header_type = (int)H_ROI;

    int bytes_sent = 0;

    int coord_count = roi.size();

    // Header
    if (send(sock_fdesc_conn_, (char*)&header_type, sizeof(header_type), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending header");
        return false;
    }

    // Send ROI coordinate count
    if (send(sock_fdesc_conn_, (char*)&coord_count, sizeof(coord_count), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending ROI coordinate string size");
        return false;
    }

    // Send ROI coordinates
    if (send(sock_fdesc_conn_, (char*)roi.c_str(), coord_count, 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending ROI coordinate count");
        return false;
    }

    return true;
}

// ------------------------------------------------------------------------------------------------------------------
bool SocketServer::SendImage(cv::Mat& image, int fps) {

    int matType = image.type();// & CV_MAT_DEPTH_MASK;

    int image_size = image.total() * image.elemSize();

    int header_type = (int)H_IMAGE;

    int bytes_sent = 0;

    if( fileChangeStatus == true && fileState == 0 ){
	header_type = (int)H_FEND;
	fileState ++;
    } else if( fileState == 1) {
	header_type = (int)H_FSTART;
	fileState ++;
    }


    // Header
    if (send(sock_fdesc_conn_, (char*)&header_type, sizeof(header_type), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending header");
        return false;
    }

    // Send number of rows to server
    if (send(sock_fdesc_conn_, (char*)&image.rows, sizeof(image.rows), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending rows");
        return false;
    }

    // Send number of cols to server
    if (send(sock_fdesc_conn_, (char*)&image.cols, sizeof(image.cols), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending cols");
        return false;
    }

    // Send number of cols to server
    if (send(sock_fdesc_conn_, (char*)&matType, sizeof(matType), 0) == -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        perror("Error sending type");
        return false;
    }

    if(header_type == H_FSTART) {
        if (send(sock_fdesc_conn_, (char*)&fps, sizeof(fps), 0) == -1) {
            printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
            perror("Error sending fps");
            return false;
        }
    }

    cv::Mat out = image;
    
    if (image.isContinuous() == false)
    {
        image.copyTo(out);
    }

    bytes_sent = send(sock_fdesc_conn_, out.data, image_size, 0);

    if (bytes_sent == image_size)
        return true;

    return false;
}

// ------------------------------------------------------------------------------------------------------------------
/*bool SocketServer::ReceiveResponse(std::string& resp)
{
    int length = 0;
    ssize_t num_bytes = recv(sock_fdesc_conn_, (char*)&length, sizeof(int), 0);

    if (num_bytes <= -1) {
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
        return false;
    }

    resp.resize(length);

    // Save image data to buffer
    for (int i = 0; i < length; i += num_bytes) {

        num_bytes = recv(sock_fdesc_conn_, &resp[i], length - i, 0);

        if (num_bytes == -1) {
            printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
            printf("ERROR!: recv failed\n");
            return false;
        }
    }
    return true;
}*/
// ------------------------------------------------------------------------------------------------------------------
/*bool SocketServer::SendResponse(const std::string& resp)
{
    // Header

    int length = (int)resp.length();

    std::string out(resp.length() +sizeof(int),0);

    memcpy(&out[0],&length,sizeof(int));
    memcpy(&out[sizeof(int)],&resp[0],length);

    if (send(sock_fdesc_conn_, &out[0],out.length()*sizeof(char), 0) == -1) {
        perror("Error sending response");
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        return false;
    }


    return true;

}*/
// ------------------------------------------------------------------------------------------------------------------
HeaderType SocketServer::ReceiveProcessedData(ProcessedData_t &pdata)
{
    ssize_t bytes_recv = 0;

    HeaderType header_type = ReceiveHeader();

    if(header_type != H_DATA){
        return header_type;
    }

#if 1
    if((bytes_recv=recv(sock_fdesc_conn_, &pdata, sizeof(ProcessedData_t), 0)) == -1){
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_,bytes_recv);
        return H_ERROR;
    }
#else

    if((bytes_recv=recv(sock_fdesc_conn_, &(pdata.objectCount), sizeof(pdata.objectCount), 0)) == -1){
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_,bytes_recv);
        return H_ERROR;
    }

    if((bytes_recv=recv(sock_fdesc_conn_, &(pdata.eventCount), sizeof(pdata.eventCount), 0)) == -1){
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_,bytes_recv);
        return H_ERROR;
    }

    if((bytes_recv=recv(sock_fdesc_conn_, pdata.objects, sizeof(pdata.objects), 0)) == -1){
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_,bytes_recv);
        return H_ERROR;
    }

    if((bytes_recv=recv(sock_fdesc_conn_, &(pdata.events), sizeof(pdata.events), 0)) == -1){
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_,bytes_recv);
        return H_ERROR;
    }

    if((bytes_recv=recv(sock_fdesc_conn_, &(pdata.isMotionInsideROI), sizeof(pdata.isMotionInsideROI), 0)) == -1){
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_,bytes_recv);
        return H_ERROR;
    }

    if((serverVersion >= TH_VERSION_DOI) && ((bytes_recv=recv(sock_fdesc_conn_, &(pdata.isMotionInsideDOI), sizeof(pdata.isMotionInsideDOI), 0)) == -1)){
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n"
                       "sock_fdesc: %d\n"
                       "bytes_recv: %zu\n", sock_fdesc_conn_,bytes_recv);
        return H_ERROR;
    }
#endif

    return header_type;
}
// ------------------------------------------------------------------------------------------------------------------
std::string SocketServer::ReceiveMACId()
{
    ssize_t bytes_recv = 0;
    std::string mac = "";

    std::vector<char> recvBuff(MAC_LENGTH);

    // Save image data to buffer
    bytes_recv = recv(sock_fdesc_conn_, recvBuff.data(), MAC_LENGTH, 0);

    if (bytes_recv == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
        return std::string();
    }
    
    mac.assign(recvBuff.data() ,MAC_LENGTH);
    
    recvBuff.clear();

    return mac;
}
// -------------------------------------------------------------------------------------------------------------------
AlgoType SocketServer::ReceiveAlg()
{
    ssize_t bytes_recv = 0;
    int  alg = TH_ALG_UNKNOWN;

    if((bytes_recv=recv(sock_fdesc_conn_, (char*) &alg, sizeof(int),0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
    }

    return (AlgoType)alg;
}

bool SocketServer::ReceiveROIState()
{
    ssize_t bytes_recv = 0;
    bool state = true;

    if((bytes_recv=recv(sock_fdesc_conn_, (char*) &state, sizeof(bool),0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
    }

    return state;
}

bool SocketServer::ReceiveDOIState()
{
    ssize_t bytes_recv = 0;
    bool state = true;

    if((bytes_recv=recv(sock_fdesc_conn_, (char*) &state, sizeof(bool),0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
    }

    return state;
}

bool SocketServer::ReceiveImageResolution(int *width, int *height)
{
    ssize_t bytes_recv = 0;
    if((bytes_recv=recv(sock_fdesc_conn_, (char*) width, sizeof(int),0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
	return false;
    }
    if((bytes_recv=recv(sock_fdesc_conn_, (char*) height, sizeof(int),0)) == -1) {
        printf("ERROR!: recv failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        printf("ERROR!: recv failed\n");
	return false;
    }
    return true;
}

// ------------------------------------------------------------------------------------------------------------------
bool SocketServer::SendClipSize(int32_t segmentation_size)
{
    int bytes_sent = 0;

    // Send segmentation size
    if ((bytes_sent = send(sock_fdesc_conn_, (char*)&segmentation_size, sizeof(segmentation_size), 0)) == -1) {
        perror("Error sending data");
        printf("ERROR!: send failed: (%s:%d)\n", __FUNCTION__, __LINE__);
        return false;
    }
    return true;
}
// -------------------------------------------------------------------------------------------------------------------
bool SocketServer::SetFileChangeStatus(bool status)
{
    fileChangeStatus = status;
    if (status && (fnum == 0)) {
        fileState = 1;
        fnum++;
    } else if(status){
        fileState = 0;
        fnum++;
    }
    return true;
}

