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

#include "TestHarnessServer.hpp"

THServer *thInstance = NULL;

void sigHandler(int signal)
{
    if(thInstance)
        thInstance -> THServerStop();
}

int main(int argc, char** argv)
{
    std::string path;

    if(argc < 2){
        std::cout << "THServer requires path to config files" << std::endl;
        return -1;
    }

    path = argv[1];
    umask(0000);

    std::signal(SIGINT, sigHandler);

    thInstance = THServer::THCreateServerInstance();

    if(!thInstance){
        std::cout << "Error creating Server instance" << std::endl;
        return -1;
    }

    if(!thInstance -> THServerInit(path)){
        std::cout << "Error initializing THServer" << std::endl;
        return -1;
    }

    if(!thInstance -> THServerStart()){
        std::cout << "Test Harness server failed to start" << std::endl;
        return -1;
    }

    while(thInstance ->THProcessFrames());
    exit(0);
}
