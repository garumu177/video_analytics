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

#ifndef _SOCKETS_SOCKET_COMMON_H
#define _SOCKETS_SOCKET_COMMON_H

#define OD_MAX_NUM 32
#define EV_MAX_NUM 32

enum HeaderType
{
    H_END = 0,
    H_ERROR = 1,
    H_IMAGE = 2,
    H_START = 3,
    H_DATA = 4,
    H_WAITING = 5,
    H_FSTART = 6,
    H_ROI = 7,
    H_DD_RESULT = 8,
    H_FEND = 9,
    H_DOI = 10,
    H_STNLD_STATUS=11
};

/** To get the video frame source
  *
  */
enum ClientType
{
    TH_UNKNOWN = 0,
    TH_CAMERA = 1,
    TH_VIDEO = 2
};

/** To get the Algorithm Type
  *
  */
enum AlgorithmType
{
    TH_ALG_UNKNOWN = 0,
    TH_ALG_IV_ENGINE = 1,
    TH_ALG_RDKC_ENGINE = 2,
};
    

/** header provided with each socket message
 *
 */
struct MessageHeader
{
    unsigned short header_type;
    int rows;
    int cols;
    int mat_type;
};

/** objects detected after processing
  *
  */
struct Object_s
{
    int lcol;
    int rcol;
    int trow;
    int brow;
    int object_class;
};

/** events detected after processing
  *
  */
struct Event_s
{
    int event_type;
};

struct ProcessedData_s
{
    /* number of objects detected on a frame */
    int objectCount;
    /* number of events detected on a frame */
    int eventCount;
    Object_s objects[OD_MAX_NUM];
    Event_s events[EV_MAX_NUM];
    bool isMotionInsideROI;
    bool isMotionInsideDOI;
};

struct TestHarnessInfo_s
{

    cv::Mat frame;
    ProcessedData_s pData;
};

#endif //_SOCKETS_SOCKET_COMMON_H
