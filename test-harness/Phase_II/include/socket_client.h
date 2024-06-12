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

#ifndef SOCKET_CLIENT_HPP
#define SOCKET_CLIENT_HPP

#include <opencv2/core.hpp>
#include "socket_common.h"
#include "RdkCTHManager.h"

/** Simple socket client
 *
 */
class SocketClient {

 private:
    /// The dim read from the last header
    cv::Size2i image_dims_;
    /// the mat type read from the last header
    int mat_type_;

    /** hostname */
    const char* hostname_;
    /** port to use */
    int port_;
    /** socket desc */
    int socket_fdesc_;

 public:
    /** constructor
     *
     * @param hostname The ip address or host name of the server
     * @param port  The port to use
     */
    SocketClient(const char* hostname, int port);
    ~SocketClient();
    /**
     * Connect to the the server
     */
    bool Connect();
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
    /** Recieve the next image or header
    *
    * @param image the output image
    * @return
    */
    HeaderType ReceiveImage(cv::Mat& image, int &fps);

    /** Sends image data to the server
     *
     * @param image The opencv MAT
     */
    bool SendImage(cv::Mat& image);
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

    /** sends ClientType
     *
     */
    bool SendClientType(ClientType cType);

    /** sends Algorithm
     *
     */
    bool SendAlgorithm(AlgorithmType alg);

    /** sends server version
     *
     */
    bool SendServerVersion();

    /** sends MAC id
     *
     */
    bool SendMacId(const std::string& mac);

    /** sends image resolution
     *
     */
    bool SendImageResolution(int width, int height);

    /** sends meta data
     *
     */
    bool SendTestHarnessMetaData(ProcessedData_s *data);

    /** sends ROI send state
     *
     */
    bool SendROIState(bool state);

    /** sends DOI send state
     *
     */
    bool SendDOIState(bool state);

    bool SendDeliveryResult(th_deliveryResult result);

    bool SendStnUploadStatus(th_smtnUploadStatus stnStatus);

    std::string ReceiveROI();

    bool ReceiveDOI(int &threshold);

    bool ReceiveClipSize(int32_t *clipsize);

    /** close the socket connection
     *
     */
    void Close();    
};

#endif
