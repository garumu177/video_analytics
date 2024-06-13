#!/bin/bash

set -e

# Function to print an error message and exit
function error_exit {
    echo "$1" 1>&2
    exit 1
}

BUILD_ESSENTIAL="no"
DOWNLOAD_COMPONENTS="no"
MAKE_CLEAN="yes"
BUILD_FFMPEG="no"
BUILD_OPENCV="no"
BUILD_CJSON="no"
BUILD_THSERVER="yes"

CLEAN_FFMPEG="no"

current_path=$(pwd)
ffmpeg_build_path="$HOME/ffmpeg_build"
bin_path="$HOME/bin"

# Preparing Prefix path for opensource components
mkdir -p opensource
export PREFIX_PATH=${PREFIX_PATH-`readlink -m ./opensource`}
PREFIX_FOLDER=`shopt -s extglob; echo ${PREFIX_PATH%%+(/)}`
echo "--------PREFIX PATH IS --------- $PREFIX_FOLDER"

export SERVER_PATH=${SERVER_PATH-`readlink -m ./server`}
SERVER_FOLDER=`shopt -s extglob; echo ${SERVER_PATH%%+(/)}`
echo "--------SERVER_PATH PATH IS --------- $SERVER_PATH"

#Preparing build folder
mkdir -p build
mkdir -p build/lib
export BUILD_PATH=${BUILD_PATH-`readlink -m ./build`}
BUILD_FOLDER=`shopt -s extglob; echo ${BUILD_PATH%%+(/)}`
echo "--------BUILD_PATH PATH IS --------$BUILD_PATH"


if [ "$BUILD_ESSENTIAL" == "yes" ]; then
    echo "Updating package list..."
    sudo dnf update -y || error_exit "Failed to update package list"

    echo "Installing essential packages..."
    sudo dnf install -y epel-release || error_exit "Failed to install EPEL release"
    sudo dnf install -y nasm yasm pkgconfig git cmake gcc gcc-c++
fi

if [ "$CLEAN_FFMPEG" == "yes" ]; then
    echo "Cleaning FFmpeg related folders..."
    rm -rvf ${ffmpeg_build_path} ${bin_path}/ffmpeg ${bin_path}/ffplay ${bin_path}/ffprobe ${bin_path}/ffserver || error_exit "Failed to clean FFmpeg related folders"
    rm -rvf ${current_path}/ffmpeg_sources/x264 || error_exit "Failed to clean libx264 folder"
    rm -rvf ${current_path}/ffmpeg_sources/ffmpeg || error_exit "Failed to clean ffmpeg folder"
fi

if [ "$DOWNLOAD_COMPONENTS" == "yes" ]; then
    rm -rvf opencv cJSON server build opensource ffmpeg_sources

    echo "Cloning libx264 source from Git..."
    mkdir -p ${current_path}/ffmpeg_sources/
    cd ${current_path}/ffmpeg_sources/
    git clone https://code.videolan.org/videolan/x264.git || error_exit "Failed to clone libx264 repository"

    echo "Cloning ffmpeg source from Git..."
    cd ${current_path}/ffmpeg_sources/
    git clone https://git.ffmpeg.org/ffmpeg.git -b release/3.3 ffmpeg || error_exit "Failed to clone ffmpeg repository"

    echo "Cloning OpenCV 3.1.0 source from GitHub..."
    cd ${current_path}/
    git clone --branch 3.1.0 https://github.com/opencv/opencv.git || error_exit "Failed to clone OpenCV repository"

    echo "Cloning cJSON source from GitHub..."
    cd ${current_path}/
    git clone https://github.com/DaveGamble/cJSON.git || error_exit "Failed to clone cJSON repository"
fi

if [ "$BUILD_FFMPEG" == "yes" ]; then
    cd ${current_path}/ffmpeg_sources/x264
    echo "Configuring and compiling libx264..."
    if [ "$MAKE_CLEAN" == "yes" ]; then
        make clean
    fi
    ./configure --enable-static --disable-asm --extra-cflags="-fPIC" || error_exit "Failed to configure libx264"
    make || error_exit "Failed to compile libx264"
    sudo make install || error_exit "Failed to install libx264"

    cd ${current_path}/ffmpeg_sources/ffmpeg
    echo "Configuring ffmpeg with libx264 support..."
    if [ "$MAKE_CLEAN" == "yes" ]; then
        make clean
    fi
    PKG_CONFIG_PATH="$ffmpeg_build_path/lib/pkgconfig"
    export PKG_CONFIG_PATH
    ./configure --prefix=/usr/local --enable-gpl --enable-libx264 --enable-shared --disable-static || error_exit "Failed to configure ffmpeg"
    echo "Compiling ffmpeg..."
    make || error_exit "Failed to compile ffmpeg"
    echo "Installing ffmpeg..."
    sudo make install || error_exit "Failed to install ffmpeg"

    echo "export LD_LIBRARY_PATH=/usr/local/lib:\$LD_LIBRARY_PATH" >> ~/.bashrc
    source ~/.bashrc
fi

if [ "$BUILD_OPENCV" == "yes" ]; then
    cd ${current_path}/opencv
    rm -rf build
    mkdir build
    cd build
    echo "Configuring OpenCV..."
    PKG_CONFIG_PATH="$ffmpeg_build_path/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/local/lib"
    export PKG_CONFIG_PATH
    export LD_LIBRARY_PATH=/usr/local/lib:/home/gokulraj/manual_testharness/ffmpeg_sources/ffmpeg/libavcodec:$LD_LIBRARY_PATH
    cmake -DENABLE_PRECOMPILED_HEADERS=OFF -DCMAKE_INSTALL_PREFIX=${PREFIX_FOLDER} -DCMAKE_CXX_FLAGS="-std=c++11" \
      -DWITH_GTK=ON -DENABLE_NEON=ON -DSOFTFP=OFF -DWITH_IPP=OFF \
      -DFFMPEG_INCLUDE_DIR=/usr/local/include \
      -DFFMPEG_LIBRARIES="/usr/local/lib/libavcodec.so;/usr/local/lib/libavformat.so;/usr/local/lib/libavutil.so;/usr/local/lib/libswscale.so;/usr/local/lib/libavdevice.so;/usr/local/lib/libavfilter.so;/usr/local/lib/libpostproc.so;/usr/local/lib/libswresample.so" ..
    echo "Compiling OpenCV..."
    if [ "$MAKE_CLEAN" == "yes" ]; then
        make clean
    fi
    make || error_exit "Failed to compile OpenCV"
    echo "Installing OpenCV..."
    sudo make install || error_exit "Failed to install OpenCV"
fi

if [ "$BUILD_CJSON" == "yes" ]; then
    cd ${current_path}/cJSON
    mkdir -p build
    cd build
    echo "Configuring cJSON..."
    cmake .. -DCMAKE_INSTALL_PREFIX=${PREFIX_FOLDER} -DBUILD_SHARED_LIBS=ON -DCMAKE_C_FLAGS="-fPIC" || error_exit "Failed to configure cJSON"
    if [ "$MAKE_CLEAN" == "yes" ]; then
        make clean
    fi
    echo "Compiling cJSON..."
    make || error_exit "Failed to compile cJSON"
    echo "Installing cJSON..."
    sudo make install || error_exit "Failed to install cJSON"
    # note : this is for opensource, for rdkc was default so below part not needed
    # Rename the directory from cjson to cJSON
    if [ -d "${PREFIX_FOLDER}/include/cjson" ]; then
        sudo mv -vf ${PREFIX_FOLDER}/include/cjson ${PREFIX_FOLDER}/include/cJSON || error_exit "Failed to rename cjson to cJSON"
    fi
fi

if [ $BUILD_THSERVER == 'yes' ]; then
   echo "*************** Building THServer ***************"
   cd ${current_path}/
   export PKG_CONFIG_PATH="$PREFIX_FOLDER/lib/pkgconfig:${PREFIX_FOLDER}/lib64/pkgconfig"
    if [ "$MAKE_CLEAN" == "yes" ]; then
        make clean
    fi  
   export CXXFLAGS="-I${PREFIX_FOLDER}/include/cJSON $CXXFLAGS" 
   export LD_LIBRARY_PATH=${PREFIX_FOLDER}/lib:${PREFIX_FOLDER}/lib64:$LD_LIBRARY_PATH
   echo "raj ${PREFIX_FOLDER}"
#   make
   #make CXXFLAGS="-I${PREFIX_FOLDER}/include" LDFLAGS="-L${PREFIX_FOLDER}/lib -L${PREFIX_FOLDER}/lib64 -lcjson"
   #make CXXFLAGS="-I${PREFIX_FOLDER}/include" LDFLAGS="-L${PREFIX_FOLDER}/lib -L${PREFIX_FOLDER}/lib64 -lcjson $(pkg-config --cflags --libs opencv)"
    make CXXFLAGS="-I${PREFIX_FOLDER}/include -I${current_path}/SocketServer" \
         LDFLAGS="-L${PREFIX_FOLDER}/lib -L${PREFIX_FOLDER}/lib64 -lcjson $(pkg-config --cflags --libs opencv) -L${current_path}/SocketServer -lsocketserver"
   echo "*************** THServer Built Successfully ***************"
   echo "*************** Removing existing libraries ***************"
   rm -rvf $BUILD_FOLDER/lib/*
   echo "*************** Copying new libraries ***************"
   cp -rvf $PREFIX_FOLDER/lib/*  $BUILD_FOLDER/lib/
   cp -rvf $PREFIX_FOLDER/lib64/*  $BUILD_FOLDER/lib/
   cp -rvf ${current_path}/SocketServer/*.so $BUILD_FOLDER/lib/
   cp -rvf ${current_path}/THServer/THServer $BUILD_FOLDER/lib/
   export LD_LIBRARY_PATH=$BUILD_FOLDER/lib/
fi

echo "All installations completed successfully!"
