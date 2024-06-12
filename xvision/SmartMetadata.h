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

/**
 * @file  SmarttnMetadata.h
 * @brief Defines for Video Analytics
 *
 * Common APIs for SmarttnMetadata Wrapper
 */
#ifndef _SMARTMETADATA_H_
#define _SMARTMETADATA_H_

#define UPPER_LIMIT_BLOB_BB	5
#define INVALID_BBOX_ORD	(-1)
#include <xcv.h>

typedef struct _BoundingBox
{
    int32_t boundingBoxXOrd;
    int32_t boundingBoxYOrd;
    int32_t boundingBoxHeight;
    int32_t boundingBoxWidth;
}BoundingBox;


struct SmarttnMetadata
{

    /*SmarttnMetadata constructor*/
    SmarttnMetadata();
    /*construct new SmarttnMetadata and fill required details*/
    static SmarttnMetadata * from_xcvInterface(xcvAnalyticsEngine *engine, vai_result_t * vai_results, char * curr_time);
    /*fill rtMessage m with SmarttnMetadata of smInfo*/
    static const rtMessage to_rtMessage(const SmarttnMetadata *smInfo);

    uint64_t timestamp;
    int32_t event_type;
    double motion_level_raw;
    double motionScore;
#ifdef _OBJ_DETECTION_
    BoundingBox deliveryUnionBox;
#endif
    BoundingBox unionBox;
    BoundingBox objectBoxs [UPPER_LIMIT_BLOB_BB];
    char const* s_curr_time;
};



#endif
