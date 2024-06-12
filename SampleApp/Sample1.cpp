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

//IPC_msgq_rcv.c
#include<iostream>
#include<signal.h>
#include<string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>

#include "RdkCMsgQue.h"

using namespace std;
void ExitApp(const char *s)
{
  perror(s);
  exit(1);
}
static int g_quit_capture = 0;

/** @description: This method gets invoked when user presses Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd.
 *  @param: int
 *  @return: void
 */

static void sigstop(int a)
{
  g_quit_capture = 1;
}

int main()
{


      //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    int APPMsgQID = -1, VAMsgQID = -1;
    key_t key = -1;
    RdkCMsgQueBuf VABuffer, AppBuffer;
    size_t VAMsgQBufLen = -1;

    key = 5678;

    if ((APPMsgQID = msgget(key, 0666|IPC_CREAT)) < 0)
      ExitApp("msgget()");

      if ((VAMsgQID = msgget(VIDEO_ANALYTICS_MSQ_KEY, 0666)) < 0)
      ExitApp("msgget()");


   VABuffer.MsgType = APP_MSG; //KEY_EVENT
   VABuffer.Data.Event = EV_REGISTER_APP_MQKEY;
   cout << "VABuf.Data.Event" << VABuffer.Data.Event << "Size: "<< sizeof(VABuffer.Data.Event) <<endl;
   strcpy(VABuffer.Data.Payload , "5678");
   if (msgsnd(VAMsgQID, &VABuffer,  PAYLOAD_MAXSIZE+sizeof(VABuffer.Data.Event), IPC_NOWAIT) < 0)
    {
        ExitApp("msgsnd");
    }

//    int count = 0;
    while(!g_quit_capture)
    {
     //Receive an answer of message type MOTION_EVENT
    if (msgrcv(APPMsgQID, &AppBuffer, PAYLOAD_MAXSIZE+sizeof(AppBuffer.Data.Event), 0xfff, 0) < 0) // 0xfff:type of msg you want to receive MOTION_EVENT
      ExitApp("msgrcv");

      printf("%s\n", AppBuffer.Data.Payload);
  /*    count++;

      if(count == 25 ){
	   VABuffer.MsgType = APP_MSG; //KEY_EVENT
           VABuffer.Data.Event = EV_DEREGISTER_APP_MQKEY;
	   cout << "VABuf.Data.Event" << VABuffer.Data.Event << "Size: "<< sizeof(VABuffer.Data.Event) <<endl;
	   strcpy(VABuffer.Data.Payload , "5678");
	   if (msgsnd(VAMsgQID, &VABuffer,  PAYLOAD_MAXSIZE+sizeof(VABuffer.Data.Event), IPC_NOWAIT) < 0)
	    {
	        ExitApp("msgsnd");
	    }
	}
*/
    }
    return 0;
}
