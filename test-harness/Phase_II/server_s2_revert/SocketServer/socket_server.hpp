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

#ifndef SOCKET_SERVER_HPP
#define SOCKET_SERVER_HPP

#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <list>
#include <cctype>
#include <signal.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <opencv2/core.hpp>
#include "Common.h"


/**
 * Socket Server
 */
class SocketServer {

    private:

    /// The dim read from the last header
    cv::Size2i image_dims_;
    /// the mat type read from the last header
    int mat_type_;

    struct sockaddr_in6 server_addr_;
    struct sockaddr_in6 client_addr_;

    socklen_t client_len_;
    size_t server_addr_size_;
    int port_;

    int sock_fdesc_init_;
    int sock_fdesc_conn_;

    int32_t serverVersion;
    bool fileChangeStatus;
    int fileState;
    int fnum;

    std::list<pid_t> pid_list;

    public:
    /**
    * constructor
    * @param port the port to use
    * @param out_path
    */
    SocketServer(int port);
    /**
    * Destructor
    */
    virtual ~SocketServer();
    /** connect
    *
    */
    bool Connect();

    bool MonitorRequest();
    /** close connection
    *
    */
    void Close();
    /** Recieve the next image or header
    *
    * @param image the output image
    * @return
    */
    HeaderType ReceiveImage(cv::Mat& image);
    /** Sends image data to the server
     *
     * @param image The opencv MAT
     */
    bool SendImage(cv::Mat& image, int fps);
    /** recieves the next header
    *
    * @return A HeaderType
    */   
    HeaderType ReceiveHeader();
    /** Sends the image header information
     *
     * @param image_rows Rows in our Mat
     * @param image_cols Cols in our Mat
     * @param Opencv Mat Type such as CV_8UC4
     */
    bool SendHeader(HeaderType type);
    /** Receive the client type
      *
      */
    Client ReceiveClientType();
    /** Recieve the next image or header
      *
      * @param image the output image
      * @return
      */
    bool ReceiveResponse(std::string& resp);
     /** sends response
     *
     */
    bool SendResponse(const std::string& resp);
    /** Recieve Server version
     *
     */
    int ReceiveServerVersion(); 
    /** Recieve MAC ID
     *
     */
    std::string ReceiveMACId(); 
    /** Recieve Alg
     *
     */
    AlgoType ReceiveAlg();
    /** Recieve ROI send state
     *
     */
    bool ReceiveROIState();
    /** Recieve DOI send state
     *
     */
    bool ReceiveDOIState();
    /** Recieve Image Resolution
     *
     */
    bool ReceiveImageResolution(int *width, int* height);
    /** Receive Processed data
      *
      */
    HeaderType ReceiveProcessedData(ProcessedData_t& pdata);
    /** Set File change status
     *
     */
    bool SetFileChangeStatus(bool status);

    bool SendROI(std::string roi);
    bool SendDOI(int threshold, std::string bitmap);
    bool ReceiveDeliveryResult(DeliveryResult_t*);
    bool SendClipSize(int32_t segmentation_size);

};

#endif
