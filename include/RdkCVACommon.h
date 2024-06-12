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

#ifndef __RDKCVACOMMON_H__
#define __RDKCVACOMMON_H__

/* common includes */
#include <iostream>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctime>
#include <time.h>
#include "rdk_debug.h"

extern int enable_debug;
#define RDK_LOG_DEBUG1 (enable_debug ? (RDK_LOG_INFO) : (RDK_LOG_DEBUG))

#define DEFAULT_WIDTH 320
// 16:9 YUV data for XCAM2, and 4:3 for other platforms
#if defined(XCAM2)
#define DEFAULT_HEIGHT 180
#else
#define DEFAULT_HEIGHT 240
#endif

/* common defines */
#define DEFAULT_VA_ALG			1			/* Default VA algorithm is set as GMM */
#define DEFAULT_SCALE_FACTOR		1			/* Default scale factor is set as 1 */
#define DEFAULT_MOTION_LEVEL_PERCENTAGE	0.0f			/* Default percentage for motion level is set as 0% */
#define DEFAULT_OD			0			/* Default number of objects detected is 0 */
#define DEFAULT_EVT			0			/* Default number of events detected is 0 */
#define DEFAULT_M_I_ID			1			/* Default object id is 1 */
#define DEFAULT_M_F_CONFIDENCE		1			/* Default confidence for detected objects is 1 */
#define DEFAULT_SENSITIVITY		1			/* Default sensitivity parameter for Day/Night mode */
#define XCV_MIN_SENSITIVITY		0			/* Minimun value for xcv sensitivity */
#define XCV_MAX_SENSITIVITY		2			/* Maximum value for xcv sensitivity */
#define MAX_MOTION_LEVEL_PERCENTAGE	25			/* If the % of total pixel under motion exceeds maximum motion 
								   level percentage, then motion level will be considered as 100 */
#ifdef _ROI_ENABLED_
#define DEFAULT_XCV_ROI_OVERLAP_PERCENTAGE 0.3f                 /* Minimum overlap percentage for motion to be considered within ROI*/
#endif

#endif
