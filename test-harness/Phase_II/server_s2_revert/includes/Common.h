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

#ifndef _COMMON_H
#define _COMMON_H

#define MAC_LENGTH 17

enum ServerVersion
{
    TH_VERSION_1_0 = 1,
    TH_VERSION_1_2 = 2,
    TH_VERSION_1_3 = 3,
    TH_VERSION_DOI = 4
};

enum Status 
{
    TH_SUCCESS = 0,
    TH_FAILURE = 1,
    TH_RETRY = 2
};

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
    H_DDRESULT = 8,
    H_FEND = 9,
    H_DOI = 10
};

/** To get the video frame source
  *
  */
enum Client
{
    TH_UNKNOWN = 0,
    TH_CAMERA = 1,
    TH_VIDEO = 2
};

/** To get the Algorithm Type
  *
  */
enum AlgoType
{
    TH_ALG_UNKNOWN = 0,
    TH_ALG_IV_ENGINE = 1,
    TH_ALG_RDKC_ENGINE = 2,
};

/** objects detected after processing
  *
  */
typedef struct Object_s
{
    int lcol;
    int rcol;
    int trow;
    int brow;
    int object_class;
}Object_t;

/** events detected after processing
  *
  */
typedef struct Event_s
{
    int event_type;
}Event_t;

typedef struct ProcessedData_s
{
    /* number of objects detected on a frame */
    int objectCount;
    /* number of events detected on a frame */
    int eventCount;
    Object_t objects[32];
    Event_t events[32];
    bool isMotionInsideROI;
    bool isMotionInsideDOI;
}ProcessedData_t;

typedef struct DeliveryResult_s
{
  //uint64_t timestamp;
  int32_t fileNum;
  int32_t frameNum;
  /* The bounding boxes of each (n) person detected in the thumbnail */
  std::vector<std::vector<int>> personBBoxes;
  /* The detection score of each (n) person detected in the thumbnail */
  std::vector<double> personScores;
  /* The bounding boxes of each (n) person detected in the thumbnail */
  std::vector<std::vector<int>> nonROIPersonBBoxes;
  /* The detection score of each (n) person detected in the thumbnail */
  std::vector<double> nonROIPersonScores;
  double deliveryScore;
  int32_t maxAugScore;
  double motionTriggeredTime;
  int32_t mpipeProcessedframes;
  double time_taken;
  double time_waited;
} DeliveryResult_t;

#endif //_COMMON_H

