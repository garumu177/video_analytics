#!/bin/bash

##########################################################################
#
# Copyright 2020 Comcast Cable Communications Management, LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
#
##########################################################################

##################################################################
## Script to build TestHarness SampleApp
##  e.g. source Build.sh
##################################################################

DOWNLOAD_COMPONENTS=yes
BUILD_OPENCV=yes
BUILD_CJSON=yes
BUILD_THSERVER=yes

#Downloading all required modules from Comcast repo.
if [ $DOWNLOAD_COMPONENTS == 'yes' ]; then

rm -rvf opencv
rm -rvf cjson
rm -rvf server
rm -rvf build
rm -rvf opensource

echo "*************** Downloading Components ***************"
git clone ssh://gerrit.teamccp.com:29418/rdk/components/opensource/cjson/generic cjson
git clone ssh://gerrit.teamccp.com:29418/rdk/components/opensource/opencv-3.1.0/generic opencv
git clone ssh://gerrit.teamccp.com:29418/rdk/components/generic/video-analytics/generic video-analytics -b 2103_sprint

export VIDEO_ANALYTICS_PATH=${VIDEO_ANALYTICS_PATH-`readlink -m ./video-analytics`}
VIDEO_ANALYTICS_FOLDER=`shopt -s extglob; echo ${VIDEO_ANALYTICS_PATH%%+(/)}`   ##Prefix path to copy opensource lib
echo $VIDEO_ANALYTICS_FOLDER

mv -v $VIDEO_ANALYTICS_FOLDER/test-harness/Phase_II/server .

rm -rvf $VIDEO_ANALYTICS_FOLDER

fi

#Preparing opensouce folder for opensouce libs
mkdir -p opensource

#Preparing Prefix path for opensource components
export PREFIX_PATH=${PREFIX_PATH-`readlink -m ./opensource`}
PREFIX_FOLDER=`shopt -s extglob; echo ${PREFIX_PATH%%+(/)}`   ##Prefix path to copy opensource lib
echo $PREFIX_FOLDER

export SERVER_PATH=${SERVER_PATH-`readlink -m ./server`}
SERVER_FOLDER=`shopt -s extglob; echo ${SERVER_PATH%%+(/)}`   ##Prefix path to copy opensource lib
echo $SERVER_FOLDER

#Preparing build folder
mkdir -p build  
mkdir -p build/lib
export BUILD_PATH=${BUILD_PATH-`readlink -m ./build`}
BUILD_FOLDER=`shopt -s extglob; echo ${BUILD_PATH%%+(/)}`
echo $BUILD_FOLDER

CURRENT_PATH=$(pwd)
export PKG_CONFIG_PATH="$(pwd)/opencv/build/unix-install/"

#Building opencv
if [ $BUILD_OPENCV == 'yes' ]; then
echo "*************** Building OpenCV ***************"
cd opencv
mkdir build 
cd build
export PKG_CONFIG_PATH="$(pwd)/opencv/build/unix-install/
cmake -DCMAKE_INSTALL_PREFIX=${PREFIX_FOLDER} -DENABLE_PRECOMPILED_HEADERS=OFF -DWITH_GTK=ON -DENABLE_NEON=ON -DSOFTFP=OFF -DWITH_IPP=OFF ..     #-DINSTALL_CREATE_DISTRIB=ON .
make
make install
cd ../..
echo "*************** Opencv Built Successfully ***************"
fi

#Building Cjson
if [ $BUILD_CJSON == 'yes' ]; then
echo "*************** Building CJSON ***************"
cd cjson
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${PREFIX_FOLDER} ..
make
make install
cd ../..
echo "*************** CJSON Built Successfully ***************"
fi

#Building THServer
if [ $BUILD_THSERVER == 'yes' ]; then
echo "*************** Building THServer ***************"
cd $SERVER_FOLDER
export PKG_CONFIG_PATH=$PREFIX_FOLDER/lib/pkgconfig
make 
cd ..
echo "*************** THServer Built Successfully ***************"
fi

echo "*************** Removing existing libraries ***************"
rm -rvf $BUILD_FOLDER/lib/*

echo "*************** Copying new libraries ***************"
cp -rvf $PREFIX_FOLDER/lib/*  $BUILD_FOLDER/lib/

cp -rvf $SERVER_FOLDER/SocketServer/*.so $BUILD_FOLDER/lib/
cp -rvf $SERVER_FOLDER/THServer/THServer $BUILD_FOLDER/lib/

export LD_LIBRARY_PATH=$BUILD_FOLDER/lib/

