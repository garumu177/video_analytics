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

#######################################################
#						      #	
# Build Framework standard script for video-analytics #
#						      #
#######################################################

# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e

# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ..`}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-`readlink -m .`}
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}

#default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}
export BUILDS_DIR=$RDK_PROJECT_ROOT_PATH
export RDK_DIR=$BUILDS_DIR
export ENABLE_RDKC_LOGGER_SUPPORT=true

export STRIP=${RDK_TOOLCHAIN_PATH}/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-strip

# build BGS algorithm
export SUPPORT_OD_USING_BGS=yes

# build Test-Harness Phase 2
export BUILD_PHASE_II=yes

#build xvision process set SUPPORT_XCV_PROCESS to yes
export SUPPORT_XCV_PROCESS=yes

#build xvision with test harness
export TEST_HARNESS=no

if [ "$XCAM_MODEL" == "SCHC2" ]; then
    . ${RDK_PROJECT_ROOT_PATH}/build/components/amba/sdk/setenv2
else
    . ${RDK_PROJECT_ROOT_PATH}/build/components/sdk/setenv2
fi

if [ "$XCAM_MODEL" == "XHB1" ] || [ "$XCAM_MODEL" == "XHC3" ]; then
    . ${RDK_PROJECT_ROOT_PATH}/build/components/sdk/setenv2
else
    export PLATFORM_SDK=${RDK_TOOLCHAIN_PATH}
    export FSROOT=$RDK_FSROOT_PATH
    export FSROOT_PATH=$RDK_FSROOT_PATH
    #cross compiler
    export CC=${SOURCETOOLCHAIN}gcc
    export CXX=${SOURCETOOLCHAIN}g++
fi

if [ "$XCAM_MODEL" == "SCHC2" ] || [ "$XCAM_MODEL" == "XHB1" ] || [ "$XCAM_MODEL" == "XHC3" ]; then
    echo "Enable xStreamer by default for xCam2 and DBC"
    export ENABLE_XSTREAMER=true
else
    echo "Disable xStreamer by default for xCam and iCam2"
    export ENABLE_XSTREAMER=false
fi

if [ "$XCAM_MODEL" == "XHB1" ]; then
    echo "Enabling delivery detection by default for DBC"
    export ENABLE_OBJ_DETECTION=true
fi

# parse arguments
INITIAL_ARGS=$@

function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}

ARGS=$@

# This Function to perform pre-build configurations before building webrtc-streaming-node code
function configure()
{
    echo "Nothing to configure for video-analytics.."
}

# This Function to perform clean the build if any exists already
function clean()
{
    cd ${RDK_SOURCE_PATH}
    echo "Start Clean"
    #rm -rvf ./src/*.o
    #rm -rvf ./src/*.so
    make clean
    echo "Cleanup done on .o and .so files"
}

# This Function peforms the build to generate the webrtc.node
function build()
{
    cd ${RDK_SOURCE_PATH}
    if [ $SUPPORT_OD_USING_BGS == 'yes' ]; then    
        cd ./XCV

        cmake -D CMAKE_C_COMPILER=${CC} -D CMAKE_CXX_COMPILER=${CXX} -D CMAKE_CXX_FLAGS="-std=gnu++0x" -DOpenCV_INCLUDE_DIR=${RDK_PROJECT_ROOT_PATH}/opensource/include/opencv .
        #cmake --prefix=${PREFIX_FOLDER} -DCMAKE_INSTALL_PREFIX=${RDK_PROJECT_ROOT_PATH}/opensource -DENABLE_NEON=ON -DSOFTFP=OFF -DCMAKE_AR=${RDK_TOOLCHAIN_PATH}/bin/arm-linux-gnueabihf-ar -DCMAKE_AS=${RDK_TOOLCHAIN_PATH}/bin/arm-linux-gnueabihf-as -DCMAKE_LD=${RDK_TOOLCHAIN_PATH}/bin/arm-linux-gnueabihf-ld -DCMAKE_RANLIB=${RDK_TOOLCHAIN_PATH}/bin/arm-linux-gnueabihf-ranlib -DCMAKE_C_COMPILER=${RDK_TOOLCHAIN_PATH}/bin/arm-linux-gnueabihf-gcc -DCMAKE_CXX_COMPILER=${RDK_TOOLCHAIN_PATH}/bin/arm-linux-gnueabihf-g++ -DARM_LINUX_SYSROOT=${RDK_TOOLCHAIN_PATH}/arm-linux-gnueabihf/libc -DOpenCV_DIR=${RDK_PROJECT_ROOT_PATH}/opensource/src/opencv-3.1.0 .

        make

        cd ..
    fi 
    make
    echo "video-analyitics is built.. Starting installation..$RDK_COMPONENT_NAME"
    install
}

# This Function peforms the rebuild to generate the webrtc.node
function rebuild()
{
    clean
    build
}

# This functions performs installation of webrtc-streaming-node output created into sercomm firmware binary
function install()
{

    echo "Start video-analytics Installation"

    cd ${RDK_SOURCE_PATH}
    if [ -f "src/libvideoanalytics.so" ]; then
        cp -rvf "${RDK_SOURCE_PATH}/src/libvideoanalytics.so" $FSROOT/usr/lib
    fi

    if [ $BUILD_PHASE_I == 'yes' -a -f "test-harness/Phase_I/src/libtestharness.so" ];then
        cp -rvf "${RDK_SOURCE_PATH}/test-harness/Phase_I/src/libtestharness.so" $FSROOT/usr/lib
    fi

    if [ $BUILD_PHASE_II == 'yes' -a -f "test-harness/Phase_II/src/libtestharness.so" ];then
	cp -rvf "${RDK_SOURCE_PATH}/test-harness/Phase_II/src/libtestharness.so" $FSROOT/usr/lib
    fi

    if [ -f "./TestMD" ]; then
       [ -d $(FSROOT)/usr/local/bin ] || mkdir -p $(FSROOT)/usr/local/bin
       cp ./TestMD $FSROOT/usr/local/bin
    fi

    if [ $SUPPORT_OD_USING_BGS == 'yes' -a -f "./XCV/build/libbgs.so" ];then
       cp -rvf "${RDK_SOURCE_PATH}/XCV/build/libbgs.so" $FSROOT/usr/lib
    fi

#    if [ -f "sdk/fsroot/src/amba/prebuild/third-party/armv7-a-hf/iva/usr/lib/libivengine.so" ]; then
#        cp -rvf "${RDK_SOURCE_PATH}/../sdk/fsroot/src/amba/prebuild/third-party/armv7-a-hf/iva/usr/lib/libivengine.so" $FSROOT/usr/lib
#    fi


#    if [ -f "./third-party-iva/sys0/ivl" ]; then
#        cp -rvf "${RDK_SOURCE_PATH}/third-party-iva/sys0" $FSROOT/etc
#    fi

    if [ $SUPPORT_XCV_PROCESS == 'yes' -a -f "./xvision/libAnalytics_Comcast.so" ];then
        cp -rvf "${RDK_SOURCE_PATH}/xvision/libAnalytics_Comcast.so" $FSROOT/usr/lib
    fi

#    if [ $SUPPORT_XCV_PROCESS == 'yes' -a -f "./xvision/libAnalytics_Intellivision.so" ];then
#        cp -rvf "${RDK_SOURCE_PATH}/xvision/libAnalytics_Intellivision.so" $FSROOT/usr/lib
#    fi

    if [ -f "xvision/xvisiond" ]; then
        cp -rvf xvision/xvisiond $RDK_SDROOT/usr/local/bin/
    fi

    if [ -f "./imagetools/rdkc_snapshooter" ]; then
        cp -rvf "${RDK_SOURCE_PATH}/imagetools/rdkc_snapshooter" $RDK_SDROOT/usr/local/bin/
    fi
    if [ -f "./imagetools/libimagetools.so" ]; then
        cp -rvf "${RDK_SOURCE_PATH}/imagetools/libimagetools.so" $RDK_SDROOT/usr/lib
    fi
    if [ -f "xvision/data/Device.X_RDKCENTRAL-COM_Camera.MotionDetection.RegionControl.json" ]; then
	mkdir -p ${RDK_FSROOT_PATH}/etc/model
        cp $RDK_SOURCE_PATH/xvision/data/*.json $RDK_FSROOT_PATH/etc/model/.
    fi

    echo "video-analytics Installation is done"
}

function setxvision()
{
    echo "setxvision - Xvision enabled build"
    export SUPPORT_XCV_PROCESS=yes
}

function setxvisionTH()
{
    echo "setxvisiontestharness - Xvision test harness  enabled build"
    export TEST_HARNESS=yes
}

# This function disables XSTREAMER flag for Hydra
function setHydra()
{
    echo "setHydra - Disable xStreamer"
    export ENABLE_XSTREAMER=false
}

# This function disables XSTREAMER flag for Hydra
function setObjDetection()
{
    echo "setHydra - enable delivery detection"
    export ENABLE_OBJ_DETECTION=true
}

# This function disables XSTREAMER flag for Hydra
function setDetectionTH()
{
    echo "setHydra - enable Testharness withdelivery detection"
    export ENABLE_OBJ_DETECTION=true
    export TEST_HARNESS=yes
}

# run the logic
#these args are what left untouched after parse_args
HIT=false

for i in "$@"; do

    echo "$i"
    case $i in
        enableHydra)  HIT=true; setHydra ;;
        enableDeliveryDetection) HIT=true; setObjDetection ;;
        enableDetectionTH) HIT=true; setDetectionTH ;;
        xvisionTH)  HIT=true; setxvisionTH ;;
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done

# if not HIT do build by default
if ! $HIT; then
  build
fi


