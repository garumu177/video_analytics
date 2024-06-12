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

#include "THInterface.h"

THInterface::THInterface()
{

}

THInterface::~THInterface()
{

}

int THInterface::THInit(int width, int height)
{
    th_objects = (th_iObject *)malloc(sizeof(th_iObject) * OD_MAX_NUM);
    if(NULL == th_objects)
    {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","malloc error!\n");
	return VA_FAILURE;
    }

    th_events = (th_iEvent *)malloc(sizeof(th_iEvent) * MAX_EVENTS_NUM);
    if(NULL == th_events)
    {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","malloc error!\n");
	return VA_FAILURE;
    }

    if (VA_SUCCESS != rdkc_get_user_setting(NW_INTERFACE, nw_if_usr)) {
	nw_if = (char*)rdkc_envGet(NW_INTERFACE);
    }
    else {
	nw_if = nw_if_usr;
    }

    if(VA_FAILURE == RdkCTHCheckConnStatus(nw_if)) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","Error checking connection status\n");
	return VA_FAILURE;
    }

    strncpy(TH_Engine, "RDKCVA",MINSIZE);
    TH_Engine[MINSIZE] = '\0';

    /* Inform Test-harness about which VA_Engine  is used */
    if( VA_FAILURE == RdkCTHSetVAEngine(TH_Engine) ) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","Could not set VA Engine\n");
    }

    /* Inform Test-harness about image resolution from camera */
    if( VA_FAILURE == RdkCTHSetImageResolution(width, height) ) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","Could not set Image resolution\n");
    }

    if(VA_SUCCESS != RdkCTHInit())
    {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","Error iniitalizing Test Harness\n");
	return VA_FAILURE;
    }
    return VA_SUCCESS;
}

int THInterface::THGetFrame(iImage *plane0, iImage *plane1, int &fps)
{
	int return_val = RdkCTHGetFrame(&th_plane0, &th_plane1, fps);
	int ret  = return_val;
	if( RDKC_FILE_START == return_val) {
		if( RdkCTHGetResetAlgOnFirstFrame() == true ){
		    return_val = RdkCVAResetAlgorithm();
		    if( VA_SUCCESS != return_val ){
			RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): Not able to reset algorithm\n",__FUNCTION__, __LINE__);
		    }
		}
	}
    	else if( VA_FAILURE == return_val ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","No frames received from test-harness to process\n");
		return VA_FAILURE;
    	}
	convert_to_IAV_planes(plane0, plane1);
	return ret;
}

int THInterface::THProcessFrame()
{
	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.XCV","send required data to test-harness...\n");
	if(RdkCTHGetFileFeedEnabledParam() == false ) {
		if( VA_FAILURE == RdkCTHPassFrame(&th_plane0, &th_plane1) ) {
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","Not able to provide frame to the test-harness. Continue with next frame.\n");
		}
	}
	if( VA_FAILURE == RdkCTHProcess(th_object_count,th_event_count,th_objects,th_events, th_is_motion_inside_ROI, th_is_motion_inside_DOI) ) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","Test-harness failed to process data. Continue with next frame.\n");
	}
	return VA_SUCCESS;
}
/** @description: reset objects and event count for test-harness
 *  @param: void
 *  @return: int
 */
int THInterface::reset_TH_object_event()
{
        th_object_count = 0;
        th_event_count = 0;
	return VA_SUCCESS;
}

/** @description: convert iObject to th_Object
 *  @param[in] obj_ptr: structure iObject
 *  @param[in] objectsCount: number of objects detected
 *  @return: int
 */
int THInterface::convert_to_TH_object(iObject *obj_ptr, int objectsCount)
{
        th_objects_ptr = th_objects;
        th_object_count = objectsCount;
        while(objectsCount-->0) {
                th_objects_ptr -> th_m_i_ID = obj_ptr -> m_i_ID;
                th_objects_ptr -> th_m_e_state = (th_eObjectState) obj_ptr -> m_e_state;
                th_objects_ptr -> th_m_f_confidence = obj_ptr -> m_f_confidence;
                th_objects_ptr -> th_m_u_bBox.th_m_i_rCol = obj_ptr -> m_u_bBox.m_i_rCol;
                th_objects_ptr -> th_m_u_bBox.th_m_i_lCol = obj_ptr -> m_u_bBox.m_i_lCol;
                th_objects_ptr -> th_m_u_bBox.th_m_i_bRow = obj_ptr -> m_u_bBox.m_i_bRow;
                th_objects_ptr -> th_m_u_bBox.th_m_i_tRow = obj_ptr -> m_u_bBox.m_i_tRow;
                th_objects_ptr -> th_m_e_objCategory = (th_eObjectCategory) obj_ptr -> m_e_objCategory;
                th_objects_ptr -> th_m_e_class = (th_eObjectClass) obj_ptr -> m_e_class;

                obj_ptr++;
                th_objects_ptr++;
        }
	return VA_SUCCESS;
}

/** @description: convert iEvent to th_Event
 *  @param[in] evt_ptr: structure iEvent
 *  @param[in] eventCount: number of events detected
 *  @return: int
 */
int THInterface::convert_to_TH_event(iEvent *evt_ptr, int eventCount)
{
        th_events_ptr = th_events;
        th_event_count = eventCount;
        while(eventCount-->0) {
                th_events_ptr -> th_m_e_type = (th_eEventType) evt_ptr -> m_e_type;
                th_events_ptr -> th_m_i_reserve1 = evt_ptr -> m_i_reserve1;
                th_events_ptr -> th_m_i_reserve2 = evt_ptr -> m_i_reserve2;
                th_events_ptr -> th_m_i_reserve3 = evt_ptr -> m_i_reserve3;
                th_events_ptr -> th_m_i_reserve4 = evt_ptr -> m_i_reserve4;
                th_events_ptr -> th_m_dt_time.th_m_i_year = evt_ptr -> m_dt_time.m_i_year;
                th_events_ptr -> th_m_dt_time.th_m_i_month = evt_ptr -> m_dt_time.m_i_month;
                th_events_ptr -> th_m_dt_time.th_m_i_day = evt_ptr -> m_dt_time.m_i_day;
                th_events_ptr -> th_m_dt_time.th_m_i_hour = evt_ptr -> m_dt_time.m_i_hour;
                th_events_ptr -> th_m_dt_time.th_m_i_minute = evt_ptr -> m_dt_time.m_i_minute;
                th_events_ptr -> th_m_dt_time.th_m_i_second = evt_ptr -> m_dt_time.m_i_second;

                evt_ptr++;
                th_events_ptr++;
        }
	return VA_SUCCESS;
}

/** @description: convert iImage plane0 and plane1 to th_Image plane0 and plane1
 *  @param[in] plane0: y-plane
 *  @param[in] plane1: uv-plane
 *  @return: int
 */
int THInterface::convert_to_TH_planes(iImage *plane0, iImage *plane1)
{
        th_plane0.data   = plane0 -> data;
        th_plane0.size   = plane0 -> size;
        th_plane0.width  = plane0 -> width;
        th_plane0.height = plane0 -> height;
        th_plane0.step   = plane0 -> step;

        th_plane1.data   = plane1 -> data;
        th_plane1.size   = plane1 -> size;
        th_plane1.width  = plane1 -> width;
        th_plane1.height = plane1 -> height;
        th_plane1.step   = plane1 -> step;
	return VA_SUCCESS;
}

/** @description: convert th_Image plane0 and plane1 to iImage plane0 and plane1
 *  @param[out] plane0: y-plane
 *  @param[out] plane1: uv-plane
 *  @return: int
 */
int THInterface::convert_to_IAV_planes(iImage *plane0, iImage *plane1)
{
        plane0 -> data   = th_plane0.data;
        plane0 -> size   = th_plane0.size;
        plane0 -> width  = th_plane0.width;
        plane0 -> height = th_plane0.height;
        plane0 -> step   = th_plane0.step;

        plane1 -> data   = th_plane1.data;
        plane1 -> size   = th_plane1.size;
        plane1 -> width  = th_plane1.width;
        plane1 -> height = th_plane1.height;
        plane1 -> step   = th_plane1.step;
	return VA_SUCCESS;
}

bool THInterface::THIsROICoordinatesChanged()
{
	return RdkCTHIsROICoordinatesChanged();
}

std::string THInterface::THGetROICoordinates()
{
	return RdkCTHGetROICoordinates();
}

bool THInterface::THIsDOIChanged()
{
	return RdkCTHIsDOIChanged();
}

bool THInterface::THGetDOIConfig(int &threshold)
{
	return RdkCTHGetDOIConfig(threshold);
}

void THInterface::THSetROIMotion(bool is_motion_inside_ROI)
{
	th_is_motion_inside_ROI = is_motion_inside_ROI;
	return;
}

void THInterface::THSetDOIMotion(bool is_motion_inside_DOI)
{
	th_is_motion_inside_DOI = is_motion_inside_DOI;
	return;
}

int THInterface::THRelease()
{
        RdkCTHRelease();

        if(th_objects) {
                free(th_objects);
                th_objects = NULL;
        }
        if(th_events) {
                free(th_events);
                th_events = NULL;
       }
	return VA_SUCCESS;
}

bool THInterface::THGetFileFeedEnabledParam()
{
	bool file_feed_status = RdkCTHGetFileFeedEnabledParam();
	return file_feed_status;
}

int THInterface::THWriteFrame(std::string filename)
{
	return RdkCTHWriteFrame(filename);
}

int THInterface::THAddDeliveryResult(th_deliveryResult result)
{
	return RdkCTHAddDeliveryResult(result);
}
 int THInterface::THAddUploadStatus(th_smtnUploadStatus stnStatus)
{

	return RdkCTHAddSTNUploadStatus(stnStatus);

}

int THInterface::THGetClipSize(int32_t *clip_size)
{
	return RdkCTHGetClipSize(clip_size);
}

/*
int main()
{
	THInterface th;
	return 0;
}
*/
