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

#ifndef RDKC_TEST_HARNESS_H
#define RDKC_TEST_HARNESS_H
#include <stdbool.h>
/*    Test-Harness manager   */
#include "RdkCTHManager.h"

#define PTS_INTERVAL            9001    /* Presentation time stamp inetrval */

int RdkCTHInit();
int RdkCTHCheckConnStatus(const char *ifname);
bool RdkCTHGetFileFeedEnabledParam();
bool RdkCTHGetResetAlgOnFirstFrame();
int RdkCTHGetFrame(th_iImage *plane0, th_iImage *plane1, int &fps);
int RdkCTHPassFrame(th_iImage *plane0, th_iImage *plane1);
int RdkCTHProcess( int objCount, int evtCount, th_iObject *th_objects, th_iEvent *th_events, bool th_isMotionInsideROI, bool th_isMotionInsideDOI);
int RdkCTHSetVAEngine(const char *VAEngine);
int RdkCTHSetImageResolution(int width, int height);
int RdkCTHRelease();			/* Releases all the occupied memory */
bool RdkCTHIsROICoordinatesChanged();
std::string RdkCTHGetROICoordinates();
bool RdkCTHIsDOIChanged();
bool RdkCTHGetDOIConfig(int &threshold);
int RdkCTHWriteFrame(std::string filename);
int RdkCTHAddDeliveryResult(th_deliveryResult result);
int RdkCTHAddSTNUploadStatus(th_smtnUploadStatus stnStatus);
int RdkCTHGetClipSize(int32_t *clip_size);
#endif   //RDKC_TEST_HARNESS_H
