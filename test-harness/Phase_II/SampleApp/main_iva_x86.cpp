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

#include <iostream>
#include <cstdlib>
#include <new>

#ifdef __cplusplus
extern "C" {
#endif

#include "RdkCTestHarness.h"

#ifdef __cplusplus
}
#endif

#include "RdkCTHManager.h"
#include "RdkCVAManager.h"
#include "RdkCVideoAnalytics.h"

#define RDKC_SUCCESS  0
#define RDKC_FAILURE -1

#define OD_MAX_NUM    32

#define PTS_INTERVAL  9001

th_iObject *th_objects = NULL;       /* Contains array of object details for test-harness */
th_iObject *th_objects_ptr = NULL;   /* Pointer to point at array of object for test-harness */
th_iEvent  *th_events = NULL;        /* Contains event details for test-harness */
th_iEvent  *th_events_ptr = NULL;    /* Pointer to point at event details for test-harness */
th_iImage  th_plane0 ;               /* test-harness y-plane */
th_iImage  th_plane1 ;               /* test-harness uv-plane */
int th_object_count = 0;
int th_event_count = 0;

int hd_mode = -1;                    /* Human Detection  --> disabled (default) */
int fd_mode = -1;                    /* Face Detection   --> disabled (default) */
int td_mode = -1;                    /* Tamper Detection --> disabled (default) */
int scene = 1;                       //Remove if not required
int sen = 8;                         //Remove if not required

iObject *objects = NULL;             /* Contains array of object details for VA Engine */
iObject *obj_ptr = NULL;             /* Pointer to point at array of object for VA Engine */
iEvent *events = NULL;               /* Contains event details for VA Engine */
iEvent *evt_ptr = NULL;              /* Pointer to point at event details for VA Engine */
int objectsCount = 0;                /* Number of Objects detected */
int eventsCount = 0;                 /* Number of Events detected */
iImage plane0;                       /* test-harness y-plane */
iImage plane1;                       /* VA-Engine uv-plane */

unsigned long framePTS = 0;

bool firstLoop = true;

iDateTime dateTime;        
int timeStamp, timeStamp2;

void resetTHObjectEvent()
{
    th_object_count = 0;
    th_event_count = 0;
}

void printObject(iObject *object)
{
    //char description[64];

    //str_objCategory(object->m_e_objCategory, description, sizeof(description));
    //IVA_PRINTF("objCategory:%s, ", description);

    //str_objClass(object->m_e_class, description, sizeof(description));
    //IVA_PRINTF("objClass:%s, ", description);

    //IVA_PRINTF("m_i_ID:%d, state:%d, confidence:%f, Box.lCol:%d, Box.tRow:%d, Box.rCol:%d, Box.bRow:%d\n",
    //object->m_i_ID, object->m_e_state, object->m_f_confidence,
    //object->m_u_bBox.m_i_lCol, object->m_u_bBox.m_i_tRow, object->m_u_bBox.m_i_rCol, object->m_u_bBox.m_i_bRow);

    std::cout << "m_i_ID:" << object->m_i_ID << ", state:" << object->m_e_state << ", confidence:" << object->m_f_confidence << 
                 ", Box.lCol:" << object->m_u_bBox.m_i_lCol << ", Box.tRow:" << object->m_u_bBox.m_i_tRow << ", Box.rCol:" << 
                 object->m_u_bBox.m_i_rCol << ", Box.bRow:" << object->m_u_bBox.m_i_bRow << std::endl;

}

void printEvent(iEvent *event)
{
    //char description[64];

    //str_eventType(event->m_e_type, description, sizeof(description));
    //IVA_PRINTF("eventType:%s, time:%d.%d.%d %d:%d:%d\n", description,
    //    event->m_dt_time.m_i_year, event->m_dt_time.m_i_month, event->m_dt_time.m_i_day, event->m_dt_time.m_i_hour,
    //    event->m_dt_time.m_i_minute, event->m_dt_time.m_i_second);

    std::cout << "eventType:" << event->m_e_type << ", time" << event->m_dt_time.m_i_year << "." << event->m_dt_time.m_i_month << 
                 "." <<  event->m_dt_time.m_i_day << "." << event->m_dt_time.m_i_hour << "." << event->m_dt_time.m_i_minute << "." << 
                 event->m_dt_time.m_i_second << std::endl;

}

int setProperty()
{
    int ret = 0;

    ret = RdkCVASetProperty(I_PROP_HUMAN_ENABLED, 1.0); /* Enable Human Detection */
    if(RDKC_SUCCESS != ret){
        std::cout << "Failed to initialize  property I_PROP_HUMAN_ENABLED" << std::endl;
        return RDKC_FAILURE;
    }

    ret = RdkCVASetProperty(I_PROP_SCENE_ENABLED, 1.0); /* enable Tamper Detection */
    if(ret != RDKC_SUCCESS){
        std::cout << "Failed to initialize  property I_PROP_SCENE_ENABLED" <<std::endl;
        return RDKC_FAILURE;
    }

    return RDKC_SUCCESS;
}

void convertToIavPlanes(iImage *plane0, iImage *plane1)
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
}

void convertToTHObject(iObject *obj_ptr, int objectsCount)
{
    th_objects_ptr = th_objects;
    th_object_count = objectsCount;

    while(objectsCount-->0) {
        th_objects_ptr -> th_m_i_ID = obj_ptr -> m_i_ID;
        th_objects_ptr -> th_m_e_state = (th_eObjectState)obj_ptr -> m_e_state;
        th_objects_ptr -> th_m_f_confidence = obj_ptr -> m_f_confidence;
        th_objects_ptr -> th_m_u_bBox.th_m_i_rCol = obj_ptr -> m_u_bBox.m_i_rCol;
        th_objects_ptr -> th_m_u_bBox.th_m_i_lCol = obj_ptr -> m_u_bBox.m_i_lCol;
        th_objects_ptr -> th_m_u_bBox.th_m_i_bRow = obj_ptr -> m_u_bBox.m_i_bRow;
        th_objects_ptr -> th_m_u_bBox.th_m_i_tRow = obj_ptr -> m_u_bBox.m_i_tRow;
        th_objects_ptr -> th_m_e_objCategory = (th_eObjectCategory)obj_ptr -> m_e_objCategory;
        th_objects_ptr -> th_m_e_class = (th_eObjectClass)obj_ptr -> m_e_class;

        obj_ptr++;
        th_objects_ptr++;
    }
}

void convertToTHEvent(iEvent *evt_ptr, int eventCount)
{
    th_events_ptr = th_events;
    th_event_count = eventCount;
        while(eventCount-->0) {
                th_events_ptr -> th_m_e_type = (th_eEventType)evt_ptr -> m_e_type;
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
}

void ExitHandler() 
{
    delete[] objects;
    delete[] events;
    delete[] th_objects;
    delete[] th_events;
    
    RdkCTHRelease();
    RdkCRelease();
}
 
int main()
{
    /* allocate memory for th_object and object */
    if(std::atexit(ExitHandler)){
        std::cout << "Error registering Exit Handler" << std::endl;
        return EXIT_FAILURE;
    }

    objects = new (std::nothrow) iObject[32];

    if(objects == nullptr){
        std::cout << "Error assigning memory to objects " << std::endl;
        return EXIT_FAILURE;
    }

    events = new (std::nothrow) iEvent[32];

    if(events == nullptr){
        std::cout << "Error assigning memory to events " << std::endl;
        return EXIT_FAILURE;
    }

    th_objects = new (std::nothrow) th_iObject[32];

    if(events == nullptr){
        std::cout << "Error assigning memory to th_objects " << std::endl;
        return EXIT_FAILURE;
    }

    th_events = new (std::nothrow) th_iEvent[32];

    if(events == nullptr){
        std::cout << "Error assigning memory to th_events " << std::endl;
        return EXIT_FAILURE;
    }
    
    /* Set the algorithm being used */
    RdkCTHSetVAEngine("RDKCVA");

    /* Initialize Test Harness */
    if(RDKC_SUCCESS != RdkCTHInit()){
        std::cout << "Error iniitalizing Test Harness" << std::endl;
        return EXIT_FAILURE;
    }

    /* Set VA Engine Properties */ 
    if(setProperty() != RDKC_SUCCESS){
        std::cout << "Error setting properties for VA Engine" << std::endl;
        return EXIT_FAILURE;
    }

    while(true){

        if(firstLoop){
            RdkCVAInIt();
            firstLoop = false;
        }

        objectsCount = 0;
        resetTHObjectEvent();

        if(RdkCTHGetFileFeedEnabledParam() == true) {
            /* Do not capture frames */
            int ret = RdkCTHGetFrame(&th_plane0, &th_plane1);

            if( RDKC_SUCCESS == ret || RDKC_FILE_START == ret){
                if( RDKC_FILE_START == ret) {
                    RdkCVAReset();
                }

                convertToIavPlanes(&plane0, &plane1);
                framePTS += PTS_INTERVAL;
            }

            else if( RDKC_FAILURE == ret ) {
                std::cout << "No frames received from test-harness to process" << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            std::cout << "Please enable the video file mode, Exiting application" << std::endl;
            return EXIT_FAILURE;
        }

        RdkCVAProcessFrame( plane0.data, plane0.size, plane0.height, plane0.width );
        //val = RdkCVAGetMotionLevel();

        RdkCVAGetObjectCount(&objectsCount);

        if (objectsCount > 0)
        {
            std::cout << "*****************************************************" << std::endl;
            std::cout << "Objects Detected, ObjectCount: " << objectsCount << std::endl;

            if(objectsCount > OD_MAX_NUM) {
                std::cout << "Objects ignored: " << (objectsCount - OD_MAX_NUM) << std::endl;
            }

            RdkCVAGetObject(objects);

            obj_ptr = objects;
            convertToTHObject(obj_ptr, objectsCount);

            while(objectsCount-->0)
            {
                printObject(obj_ptr++);
            }
            std::cout << "*****************************************************" << std::endl;
        }

        RdkCVAGetEventCount(&eventsCount);
        
        if (eventsCount > 0){
            std::cout << "*****************************************************" << std::endl;
            std::cout << "Events detected, EventCount" <<  eventsCount << std::endl;

            RdkCVAGetEvent(events);

            if (events->m_e_type == eSceneChange){
                //handle the event
            }
            else if (events->m_e_type == eLightsOn){
                std::cout << "Event::eLightsOn" << std::endl;
            }
            else if (events->m_e_type == eLightsOff){
                std::cout << "Event::eLightsOff" << std::endl;
            }
            evt_ptr = events;

            convertToTHEvent(evt_ptr, eventsCount);
            
            while(eventsCount-->0)
            {
                printEvent(evt_ptr++);
            }
            std::cout << "*****************************************************" << std::endl;
        }
	if(RDKC_FAILURE == RdkCTHProcess(th_object_count,th_event_count,th_objects,th_events)){
	    std::cout << "Error processing objects and event" << std::endl;
	    return EXIT_FAILURE;
	}
    }
}

