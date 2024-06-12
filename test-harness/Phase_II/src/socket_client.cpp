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

#include "socket_client.h"
#include <iostream>
#include <memory> // unique_ptr
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <opencv2/highgui.hpp>
#include "socket_common.h"
#define BUFFLEN 250
//#include "Packer.h"

#define DOI_BITMAP_FILE "/opt/doi_bitmap.bmp"

// ------------------------------------------------------------------------------------------------------------------
SocketClient::SocketClient(const char* hostname, int port) :
    hostname_ (hostname),
    port_(port),
    mat_type_(0),
    socket_fdesc_(0) {}

// ------------------------------------------------------------------------------------------------------------------
SocketClient::~SocketClient()
{
    Close();
}
// ------------------------------------------------------------------------------------------------------------------
void SocketClient::Close() {

    if( 0 < shutdown(socket_fdesc_,SHUT_RDWR)) {
        printf("%s(%d): %s:%s\n", __FILE__, __LINE__,"shutdown",strerror(errno));
    }

    if (socket_fdesc_ > 0)
    {
        close(socket_fdesc_);
    }
    return;
}

// ------------------------------------------------------------------------------------------------------------------
bool SocketClient::Connect() {

    struct addrinfo addrinfo_hints;
    struct addrinfo* addrinfo_resp;
    bool isConnected = false;

    do {
        // Specify criteria for address structs to be returned by getAddrinfo
        memset(&addrinfo_hints, 0, sizeof(addrinfo_hints));
        addrinfo_hints.ai_socktype = SOCK_STREAM;
        addrinfo_hints.ai_family = AF_INET6;

        // Populate addr_info_resp with address responses matching hints
        if (getaddrinfo(hostname_, std::to_string(port_).c_str(),
                        &addrinfo_hints, &addrinfo_resp) != 0) {
            perror("Couldn't connect to host!, Falling back to IPV4");
            break;
        }

        // Create socket file descriptor for server
        socket_fdesc_ = socket(addrinfo_resp->ai_family, addrinfo_resp->ai_socktype,
                               addrinfo_resp->ai_protocol);
        if (socket_fdesc_ == -1) {
            perror("Error opening socket");
            free(addrinfo_resp);
            break;
        }

        // Connect to server specified in address struct, assign process to server
        // file descriptor
        if (connect(socket_fdesc_, addrinfo_resp->ai_addr,
                    addrinfo_resp->ai_addrlen) == -1) {
            perror("Error connecting to address");
            free(addrinfo_resp);
            break;
        }

        isConnected = true;

    }while(0);

    if(!isConnected) {
        printf("Couldn't connect to host!, Falling back to IPV4\n");
        // Specify criteria for address structs to be returned by getAddrinfo
        memset(&addrinfo_hints, 0, sizeof(addrinfo_hints));
        addrinfo_hints.ai_socktype = SOCK_STREAM;
        addrinfo_hints.ai_family = AF_INET;

        // Populate addr_info_resp with address responses matching hints
        if (getaddrinfo(hostname_, std::to_string(port_).c_str(),
                        &addrinfo_hints, &addrinfo_resp) != 0) {
            perror("Couldn't connect to host!");
            return false;
        }

        // Create socket file descriptor for server
        socket_fdesc_ = socket(addrinfo_resp->ai_family, addrinfo_resp->ai_socktype,
                               addrinfo_resp->ai_protocol);
        if (socket_fdesc_ == -1) {
            perror("Error opening socket");
            free(addrinfo_resp);
            return false;
        }

        // Connect to server specified in address struct, assign process to server
        // file descriptor
        if (connect(socket_fdesc_, addrinfo_resp->ai_addr,
                    addrinfo_resp->ai_addrlen) == -1) {
            perror("Error connecting to address");
            free(addrinfo_resp);
            return false;
        }

        isConnected = true;
    }

    free(addrinfo_resp);

    return true;
}
// ------------------------------------------------------------------------------------------------------------------

bool SocketClient::SendHeader(HeaderType type)
{
  // Send number of rows to server
  if (send(socket_fdesc_, (char*)&type, sizeof(type), 0) == -1) {
    perror("Error sending rows");
    return false;
  }
  return true;
}
// ------------------------------------------------------------------------------------------------------------------
HeaderType SocketClient::ReceiveHeader() {

    ssize_t bytes_sent = 0;
    size_t dims_size = 0;

    int cols = 0;
    int rows = 0;
    int header_type = H_WAITING;


    if ((bytes_sent = recv(socket_fdesc_, (char *) &header_type, sizeof(header_type), 0)) == -1) {
        printf("ERROR!: recv failed\n"
                       "socket_fdesc_: %d\n"
                       "image_size: %zu\n"
                       "bytes_sent: %zu\n", socket_fdesc_, dims_size, bytes_sent);
        return H_ERROR;
    }

    if (header_type != (int) H_IMAGE && header_type != (int) H_FSTART)
        return (HeaderType) header_type;

    if ((bytes_sent = recv(socket_fdesc_, (char *) &rows, sizeof(rows), 0)) == -1) {
        printf("ERROR!: recv failed\n"
                       "socket_fdesc_: %d\n"
                       "image_size: %zu\n"
                       "bytes_sent: %zu\n", socket_fdesc_, dims_size, bytes_sent);
        return H_ERROR;
    }

    if ((bytes_sent = recv(socket_fdesc_, (char *) &cols, sizeof(cols), 0)) == -1) {
        printf("ERROR!: recv failed\n"
                       "socket_fdesc_: %d\n"
                       "image_size: %zu\n"
                       "bytes_sent: %zu\n", socket_fdesc_, dims_size, bytes_sent);
        return H_ERROR;
    }

    if ((bytes_sent = recv(socket_fdesc_, (char *) &mat_type_, sizeof(mat_type_), 0)) == -1) {
        printf("ERROR!: recv failed\n"
                       "socket_fdesc_: %d\n"
                       "image_size: %zu\n"
                       "bytes_sent: %zu\n", socket_fdesc_, dims_size, bytes_sent);
        return H_ERROR;
    }

    image_dims_ = cv::Size2i(cols, rows);
    printf("Image dimensions: [%dx%d,%d]\n", cols, rows, mat_type_);

    return (HeaderType)header_type;
}
// ------------------------------------------------------------------------------------------------------------------
bool SocketClient::SendResponse(const std::string& resp)
{
    // Header

    int length = (int)resp.length();

    std::string out(resp.length() +sizeof(int),0);

    memcpy(&out[0],&length,sizeof(int));
    memcpy(&out[sizeof(int)],&resp[0],length);

    if (send(socket_fdesc_, &out[0],out.length()*sizeof(char), 0) == -1) {
        perror("Error sending response");
        return false;
    }


    return true;

}

// ------------------------------------------------------------------------------------------------------------------
bool SocketClient::ReceiveResponse(std::string& resp)
{
    int length = 0;
    ssize_t num_bytes = recv(socket_fdesc_, (char*)&length, sizeof(int), 0);

    if (num_bytes <= -1) {
        printf("ERROR!: recv failed\n");
        return false;
    }

    resp.resize(length);

    // Save image data to buffer
    for (int i = 0; i < length; i += num_bytes) {

        num_bytes = recv(socket_fdesc_, &resp[i], length - i, 0);

        if (num_bytes == -1) {
            printf("ERROR!: recv failed\n");
            return false;
        }
    }
    return true;
}
// ------------------------------------------------------------------------------------------------------------------
bool SocketClient::SendImage(cv::Mat& image) {

    int matType = image.type();// & CV_MAT_DEPTH_MASK;

    int image_size = image.total() * image.elemSize();

    int header_type = (int)H_IMAGE;

    // Header
    if (send(socket_fdesc_, (char*)&header_type, sizeof(header_type), 0) == -1) {
        perror("Error sending header");
        return false;
    }

    // Send number of rows to server
    if (send(socket_fdesc_, (char*)&image.rows, sizeof(image.rows), 0) == -1) {
        perror("Error sending rows");
        return false;
    }

    // Send number of cols to server
    if (send(socket_fdesc_, (char*)&image.cols, sizeof(image.cols), 0) == -1) {
        perror("Error sending cols");
        return false;
    }

    // Send number of cols to server
    if (send(socket_fdesc_, (char*)&matType, sizeof(matType), 0) == -1) {
        perror("Error sending type");
        return false;
    }


    cv::Mat out = image;
    if (image.isContinuous() == false)
    {
        image.copyTo(out);
    }

    int num_bytes = send(socket_fdesc_, out.data, image_size, 0);

    if (num_bytes > 0)
        return true;

    return false;
}
//------------------------------------------------------------------------------------------------------------------
bool SocketClient::ReceiveDOI(int &threshold) {

    ssize_t bytes_sent = 0;
    int cols = 0;
    int rows = 0;

    //Receiving doi threshold first
    bytes_sent = recv(socket_fdesc_, &threshold, sizeof(threshold), 0);
    if (bytes_sent == -1) {
        perror("Error receiving threshold");
        return false;
    }

    if ((bytes_sent = recv(socket_fdesc_, (char *) &rows, sizeof(rows), 0)) == -1) {
        perror("Error receiving rows");
        return false;
    }

    if ((bytes_sent = recv(socket_fdesc_, (char *) &cols, sizeof(cols), 0)) == -1) {
        perror("Error receiving cols");
        return false;
    }

    if ((bytes_sent = recv(socket_fdesc_, (char *) &mat_type_, sizeof(mat_type_), 0)) == -1) {
        perror("Error receiving mat");
        return false;
    }

    // Reset image
    cv::Mat image = cv::Mat::zeros(cv::Size2i(cols, rows), mat_type_);

    // Get image size
    int image_size = image.total() * image.elemSize();

    // Allocate space for image buffer
    uchar *sock_data = image.ptr<uchar>();

    // Save image data to buffer
    for (int i = 0; i < image_size; i += bytes_sent) {

        bytes_sent = recv(socket_fdesc_, sock_data + i, image_size - i, 0);

        if (bytes_sent == -1) {
            perror("Error receiving image data");
            return false;
        }
    }

    imwrite(DOI_BITMAP_FILE, image);
    return true;
}

// ------------------------------------------------------------------------------------------------------------------
std::string SocketClient::ReceiveROI() {
    char coords[BUFFLEN+1];
    int coord_size = 0;

    ssize_t num_bytes = 0;

    //Receiving the size of roi coordinates string first
    num_bytes = recv(socket_fdesc_, &coord_size, sizeof(coord_size), 0);
    if (num_bytes == -1) {
        printf("ERROR!: recv failed\n"
                       "socket_fdesc_: %d\n"
                       "roi_size: 4\n"
                       "num_bytes: %d\n", socket_fdesc_, (int)num_bytes);
    }

    if( coord_size > 0 && coord_size <= BUFFLEN ) {
        //Receive the roi coordinate string
        num_bytes = recv(socket_fdesc_, coords, coord_size, 0);
        if (num_bytes == -1) {
            printf("ERROR!: recv failed\n"
                           "socket_fdesc_: %d\n"
                           "roi_size: %d\n"
                           "num_bytes: %d\n", socket_fdesc_, coord_size,  (int)num_bytes);
        }
    }
    coords[BUFFLEN] = '\0';
    return std::string(coords);
}

// ------------------------------------------------------------------------------------------------------------------
HeaderType SocketClient::ReceiveImage(cv::Mat &image, int &fps) {

    HeaderType header_type = ReceiveHeader();

    if (header_type != H_IMAGE && header_type != H_FSTART && header_type != H_FEND)
        return (HeaderType) header_type;

    ssize_t num_bytes = 0, bytes_sent = 0;
    size_t image_size = 0;

    // Reset image
    image = cv::Mat::zeros(image_dims_, mat_type_);

    // Get image size
    image_size = image.total() * image.elemSize();

    // Allocate space for image buffer
    uchar *sock_data = image.ptr<uchar>();

    if ((header_type == (int) H_FSTART) && ((bytes_sent = recv(socket_fdesc_, (char *) &fps, sizeof(fps), 0)) == -1)) {
        printf("ERROR!: recv failed\n"
                       "socket_fdesc_: %d\n"
                       "image_size: %zu\n"
                       "bytes_sent: %zu\n", socket_fdesc_, sizeof(fps), bytes_sent);
        return H_ERROR;
    }

    // Save image data to buffer
    for (int i = 0; i < image_size; i += num_bytes) {
	
        num_bytes = recv(socket_fdesc_, sock_data + i, image_size - i, 0);

        if (num_bytes == -1) {
            printf("ERROR!: recv failed\n"
                           "i: %d\n"
                           "socket_fdesc_: %d\n"
                           "image_size: %d\n"
                           "num_bytes: %d\n", i, socket_fdesc_, (int)image_size, (int)num_bytes);
            return H_ERROR;
        }
    }
    return header_type;
}
// ------------------------------------------------------------------------------------------------------------------
bool SocketClient::SendClientType(ClientType cType)
{
    if (send(socket_fdesc_, (char*)&cType, sizeof(cType), 0) == -1) {
        perror("Error sending Client type");
        return false;
    }
    return true;
}
// ------------------------------------------------------------------------------------------------------------------
bool SocketClient::SendAlgorithm(AlgorithmType alg)
{
    if (send(socket_fdesc_, (char*)&alg, sizeof(alg), 0) == -1) {
        perror("Error sending Algorithm type");
        return false;
    }
    return true;
}
// ------------------------------------------------------------------------------------------------------------------
bool SocketClient::SendServerVersion()
{
    int32_t serverVersion = 4;

    if(send(socket_fdesc_,(char*)&serverVersion,sizeof(serverVersion),0) == -1) {
        perror("Error sending MAC ID");
        return false;
    }

    return true;
}

bool SocketClient::SendROIState(bool state)
{
    if (send(socket_fdesc_, (char*)&state, sizeof(state), 0) == -1) {
        perror("Error sending ROI state");
        return false;
    }
    return true;
}

bool SocketClient::SendDOIState(bool state)
{
    if (send(socket_fdesc_, (char*)&state, sizeof(state), 0) == -1) {
        perror("Error sending DOI state");
        return false;
    }
    return true;
}

bool SocketClient::SendMacId(const std::string& mac)
{
    int length = -1;

    if((length=mac.length()) != 17){
        perror("Incorrect MAC ID");
        return false;
    }
    if(send(socket_fdesc_,&mac[0],length*sizeof(char),0) == -1) {
        perror("Error sending MAC ID");
        return false;
    }

    return true;
}

bool SocketClient::SendTestHarnessMetaData(ProcessedData_s *data)
{
    int header_type = (int)H_DATA;

    // Header
    if (send(socket_fdesc_, (char*)&header_type, sizeof(header_type), 0) == -1) {
        perror("Error sending header");
        return false;
    }
    //meta_data
    if(send(socket_fdesc_,(char*) data,sizeof(ProcessedData_s),0) == -1) {
	perror("Error sending TestHarness meta-data");
        return false;
    }
    return true;
}

bool SocketClient::SendImageResolution(int width, int height)
{
    if (send(socket_fdesc_, (char*)&width, sizeof(int), 0) == -1) {
        perror("Error sending width");
        return false;
    }
    if (send(socket_fdesc_, (char*)&height, sizeof(int), 0) == -1) {
        perror("Error sending height");
        return false;
    }
    return true;
}

bool SocketClient::ReceiveClipSize(int32_t *clipsize)
{
    
    ssize_t num_bytes = recv(socket_fdesc_, (char*)clipsize, sizeof(int32_t), 0);

    if (num_bytes <= -1) {
        printf("ERROR!: recv failed : clipsize\n");
        return false;
    }
    return true;
}
bool SocketClient::SendStnUploadStatus(th_smtnUploadStatus stnStatus)
{
    int header_type=(int)H_STNLD_STATUS;
     // Header

    if (send(socket_fdesc_, (char*)&header_type, sizeof(header_type), 0) == -1) {
        perror("Error sending header");
        return false;
    }

    if (send(socket_fdesc_, (char*)&(stnStatus.status), sizeof(stnStatus.status), 0) == -1) {
        perror("Error sending stn upload status");
        return false;
    }
    return true;
}

bool SocketClient::SendDeliveryResult(th_deliveryResult result)
{
    int header_type = (int)H_DD_RESULT;

    // Header
    if (send(socket_fdesc_, (char*)&header_type, sizeof(header_type), 0) == -1) {
        perror("Error sending header");
        return false;
    }

    //Send DD_result
    //Send timestamp
/*    if (send(socket_fdesc_, (char*)&(result.timestamp), sizeof(result.timestamp), 0) == -1) {
        perror("Error sending timestamp");
        return false;
    }*/

    if (send(socket_fdesc_, (char*)&(result.fileNum), sizeof(result.fileNum), 0) == -1) {
        perror("Error sending fileNum");
        return false;
    }

    if (send(socket_fdesc_, (char*)&(result.frameNum), sizeof(result.frameNum), 0) == -1) {
        perror("Error sending frameNum");
        return false;
    }

    //Send personcount
    int count = result.personScores.size();
    if (send(socket_fdesc_, (char*)&count, sizeof(int), 0) == -1) {
        perror("Error sending personcount");
        return false;
    }
    for(int i = 0; i< count; i++) {
        //Send bbox 
        for(int j = 0; j < 4; j++) {
            if (send(socket_fdesc_, (char*)&(result.personBBoxes[i].at(j)), sizeof(int), 0) == -1) {
                perror("Error sending person bbox");
                return false;
            }
        }

        //Send personscore
        if (send(socket_fdesc_, (char*)&(result.personScores[i]), sizeof(double), 0) == -1) {
            perror("Error sending delivery score");
            return false;
        }
    }

    //Send nonROIpersoncount
    count = result.nonROIPersonScores.size();
    if (send(socket_fdesc_, (char*)&count, sizeof(int), 0) == -1) {
        perror("Error sending nonROIpersoncount");
        return false;
    }
    for(int i = 0; i< count; i++) {
        //Send bbox 
        for(int j = 0; j < 4; j++) {
            if (send(socket_fdesc_, (char*)&(result.nonROIPersonBBoxes[i].at(j)), sizeof(int), 0) == -1) {
                perror("Error sending non ROI person bbox");
                return false;
            }
        }

        //Send personscore
        if (send(socket_fdesc_, (char*)&(result.nonROIPersonScores[i]), sizeof(double), 0) == -1) {
            perror("Error sending person score");
            return false;
        }
    }
    //Send deliveryscore
    if (send(socket_fdesc_, (char*)&(result.deliveryScore), sizeof(result.deliveryScore), 0) == -1) {
        perror("Error sending delivery score");
        return false;
    }
    //Send maxAugscore
    if (send(socket_fdesc_, (char*)&(result.maxAugScore), sizeof(result.maxAugScore), 0) == -1) {
        perror("Error sending maxAugScore");
        return false;
    }
    //Send motionTriggeredTime
    if (send(socket_fdesc_, (char*)&(result.motionTriggeredTime), sizeof(result.motionTriggeredTime), 0) == -1) {
        perror("Error sending motionTriggeredTime");
        return false;
    }
    //Send mpipeProcessedframes
    if (send(socket_fdesc_, (char*)&(result.mpipeProcessedframes), sizeof(result.mpipeProcessedframes), 0) == -1) {
        perror("Error sending mpipeProcessedframes");
        return false;
    }
    //Send time_taken
    if (send(socket_fdesc_, (char*)&(result.time_taken), sizeof(result.time_taken), 0) == -1) {
        perror("Error sending time_taken");
        return false;
    }
    //Send time_waited
    if (send(socket_fdesc_, (char*)&(result.time_waited), sizeof(result.time_waited), 0) == -1) {
        perror("Error sending time_waited");
        return false;
    }
    return true;
}
// ------------------------------------------------------------------------------------------------------------------
