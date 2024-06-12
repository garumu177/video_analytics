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

/*************************       INCLUDES         *************************/
#include "RdkCVideoAnalytics.h"


// Creating Video Analytics object to use its features
static VideoAnalytics *VA;


static int g_quit_capture = 0;

/** @description: This method gets invoked when user presses Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd.
 *  @param: int
 *  @return: void
 */

static void sigstop(int a)
{
  g_quit_capture = 1;
	
  // freeing the Video Analytics object
  if(VA != NULL)
  {
    delete VA;
    VA = NULL;
  }
}


/***************** main() ******************************/
/**@description: Registers the Application callback to Video Analytics module and starts motion detection on successful registration.
*/
int main ()
{
  rdk_logger_init(NULL);

  //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);
  
  VA = CreateVideoAnalyticsInstance();

  // Application gets into infinite loop till user wants to quit
  while (!g_quit_capture)
  {
    usleep ( 1000000 );
  }
  
  
  return 0;
}

/****************************** EOF **************************************/
