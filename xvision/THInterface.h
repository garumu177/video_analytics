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

#ifndef __THINTERFACE_H__
#define __THINTERFACE_H__
#include <stdlib.h>
#include <string.h>
#include <string>
#include "rdk_debug.h"
#include "VAStructs.h"
#include "va_defines.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "RdkCTestHarness.h"
#include "RdkCTHManager.h"
#ifdef __cplusplus
}
#endif

#include "RdkCVAManager.h"
#include "dev_config.h"

#define MAXSIZE            		 100
#define MINSIZE                  10

class THInterface
{
private:
        iImage yuv_plane0, yuv_plane1;
        char * nw_if;
        char nw_if_usr[CONFIG_PARAM_MAX_LENGTH];
	th_iObject *th_objects;           /* Contains array of object details for test-harness */
	th_iObject *th_objects_ptr;       /* Poinetr to point at array of object for test-harness */
	th_iEvent  *th_events;            /* Contains event details for test-harness */
	th_iEvent  *th_events_ptr;        /* Poinetr to point at event details for test-harness */
	th_iImage  th_plane0;             /* test-harness y-plane */
	th_iImage  th_plane1;             /* test-harness uv-plane */
	int th_object_count;              /* test-harness object count */
	int th_event_count;               /* test-harness event count */
	char TH_Engine[MINSIZE+1];
	bool th_is_motion_inside_ROI;
	bool th_is_motion_inside_DOI;

public:
	THInterface();
	~THInterface();
	int THInit(int width,int height);
	int THGetFrame(iImage *plane0, iImage *plane1, int &fps);
	int THProcessFrame();
	int reset_TH_object_event();
	int convert_to_TH_object(iObject *obj_ptr, int objectsCount);
	int convert_to_TH_event(iEvent *evt_ptr, int eventCount);
	int convert_to_TH_planes(iImage *plane0, iImage *plane1);
	int convert_to_IAV_planes(iImage *plane0, iImage *plane1);
	bool THGetFileFeedEnabledParam();
	bool THIsROICoordinatesChanged();
	std::string THGetROICoordinates();
	bool THIsDOIChanged();
	bool THGetDOIConfig(int &threshold);
	void THSetROIMotion(bool is_motion_inside_ROI);
	void THSetDOIMotion(bool is_motion_inside_DOI);
	int THWriteFrame(std::string filename);
	int THAddDeliveryResult(th_deliveryResult result);
	int THAddUploadStatus(th_smtnUploadStatus stnStatus);
	int THGetClipSize(int32_t *clip_size);
	int THRelease();
};
#endif //__THINTERFACE_H__
