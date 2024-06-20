#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build/lib
./THServer/THServer ./THServer/THServerConfig.txt
