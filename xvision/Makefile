##########################################################################
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
##########################################################################

#CFLAGS += -DENABLE_TEST_HARNESS
PROJ_INSTALL := ${RDK_SDROOT}
ifeq ($(XCAM_MODEL), XHB1)
CFLAGS += -DOSI
CFLAGS += -DXHB1
USE_DEWARP = yes
else
ifeq ($(XCAM_MODEL), XHC3)
CFLAGS += -DOSI
CFLAGS += -DXHC3
USE_DEWARP = yes
else
SUPPORT_MXML = yes
USE_RDKCSTREAM = yes
endif
endif

ifeq ($(XCAM_MODEL), SCHC2)
USE_DEWARP = yes
endif

# Add dependent libraries
USE_OPENCV = yes
USE_CONFIGMGR = yes
USE_RFCCONFIG = yes
USE_RBUS = yes
USE_BREAKPAD = yes
USE_TELEMETRY_2 = yes
ENABLE_DIRECT_FRAME_READ := true

ifneq ($(ENABLE_XSTREAMER), true)
USE_PLUGINS = yes
else
USE_CONSUMER = yes
USE_SYSUTILS = yes
endif

USE_CONFIGUTILS = yes

include ${RDK_PROJECT_ROOT_PATH}/utility/AppsRule.mak
LIBS = $(LIBFLAGS)
CFLAGS  += -I./include
CFLAGS  += -g -std=c++14 -fPIC -Wall -Wextra

ifeq ($(TEST_HARNESS), yes)
$(info '************* TH ENABLED ********************')
CFLAGS += -DENABLE_TEST_HARNESS
# CFLAGS for Test Harness
CFLAGS  += -I$(RDK_PROJECT_ROOT_PATH)/video-analytics/test-harness/Phase_II/include
LIBS    += -L$(RDK_PROJECT_ROOT_PATH)/video-analytics/test-harness/Phase_II/src -ltestharness
endif

ifeq ($(XCAM_MODEL), SCHC2)
CFLAGS += -DXCAM2
endif

CFLAGS  += -I$(RDK_PROJECT_ROOT_PATH)/video-analytics/include
CFLAGS  += -I$(RDK_PROJECT_ROOT_PATH)/video-analytics/XCV
CFLAGS  += -I$(RDK_PROJECT_ROOT_PATH)/video-analytics/XCV/thirdparty
CFLAGS  += -I$(RDK_PROJECT_ROOT_PATH)/video-analytics/XCV/components/objValidation
CFLAGS  += -I$(RDK_PROJECT_ROOT_PATH)/video-analytics/xvision
CFLAGS  += -I$(RDK_PROJECT_ROOT_PATH)/utility/dynamicLogging/util

ifeq ($(XCAM_MODEL), SCHC2)
CFLAGS += -DXCAM2
endif

ifeq ($(ENABLE_XSTREAMER), true)
$(info( '*************XSTREAMER ENABLED**************')
CFLAGS  += -D_HAS_XSTREAM_

ifeq ($(ENABLE_DIRECT_FRAME_READ), true)
$(info( '*************DIRECT READ FRAME ENABLED**************')
CFLAGS   += -D_DIRECT_FRAME_READ_
LDFLAGS  = -Wl,-rpath-link=$(PROJ_ROOT)/vendor/img/fs/shadow_root/usr/lib
endif
endif

ifeq ($(ENABLE_OBJ_DETECTION), true)
CFLAGS += -D_OBJ_DETECTION_
endif

CFLAGS += -DRTMSG -D_ROI_ENABLED_

# Linking path for RDKC
LIBS    += -L$(RDK_PROJECT_ROOT_PATH)/video-analytics/XCV/build -lbgs
LIBS    += -L$(RDK_PROJECT_ROOT_PATH)/opensource/lib -llog4c -lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lopencv_photo -lopencv_video
LIBS    += -L$(RDK_PROJECT_ROOT_PATH)/opensource/lib -lssl -lcrypto -lopencv_core -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_objdetect -lopencv_highgui -lopencv_ml
LIBS    += -L$(RDK_PROJECT_ROOT_PATH)/utility/dynamicLogging/util -lrdkdynamiclog
LIBS    += -L$(RDK_PROJECT_ROOT_PATH)/video-analytics/src -lvideoanalytics
LIBS    += -L$(RDK_PROJECT_ROOT_PATH)/video-analytics/xvision

#-Wl,
SRC_XCV+= libAnalytics_Comcast.cpp
SRC_XVINTER += xcvInterface.cpp
SRC_IA += iavInterface.cpp
SRC_XVISION += xvisiond.cpp

ifeq ($(TEST_HARNESS), yes)
SRC_TH += THInterface.cpp
OBJ_TH = $(SRC_TH:.cpp=.o)
endif

OBJ_IAV	= $(SRC_IA:.cpp=.o)
OBJ_XCV  = $(SRC_XCV:.cpp=.o)
OBJ_XVISION  = $(SRC_XVISION:.cpp=.o)
OBJ_XVINTER = $(SRC_XVINTER:.cpp=.o)
INSTPROGS += libAnalytics_Comcast.so

RELEASE_TARGET = xvisiond
DEBUG_TARGET = xvisiond_debug

all:  libAnalytics_Comcast.so libAnalytics_Comcast_debug.so $(RELEASE_TARGET) $(DEBUG_TARGET) install

libAnalytics_Comcast.so: $(OBJ_XCV) $(OBJ_XVINTER) $(OBJ_IAV)
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -shared -o $(@)
	$(STRIP) libAnalytics_Comcast.so

libAnalytics_Comcast_debug.so: $(OBJ_XCV) $(OBJ_XVINTER) $(OBJ_IAV)
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -shared -o $(@)

ifeq ($(TEST_HARNESS), yes)
$(RELEASE_TARGET): $(OBJ_XVINTER) $(OBJ_IAV) $(OBJ_XVISION) $(OBJ_TH)
else
$(RELEASE_TARGET): $(OBJ_XVINTER) $(OBJ_IAV) $(OBJ_XVISION)
endif
	$(CXX) $(CFLAGS) -o $(@) $^ $(LIBS) $(LDFLAGS) -lAnalytics_Comcast
	$(STRIP) $(RELEASE_TARGET)

ifeq ($(TEST_HARNESS), yes)
$(DEBUG_TARGET): $(OBJ_XVINTER) $(OBJ_IAV) $(OBJ_XVISION) $(OBJ_TH)
else
$(DEBUG_TARGET): $(OBJ_XVINTER) $(OBJ_IAV) $(OBJ_XVISION)
endif
	$(CXX) $(CFLAGS) -o $(@) $^ $(LIBS) $(LDFLAGS) -lAnalytics_Comcast

install:
	$(PLATFORM_BREAKPAD_BINARY) $(DEBUG_TARGET) > $(RELEASE_TARGET).sym
	sed -i "1s/$(DEBUG_TARGET)/$(RELEASE_TARGET)/" $(RELEASE_TARGET).sym
	mv -f $(RELEASE_TARGET).sym  $(PLATFORM_SYMBOL_PATH)
	echo "Debug symbol created for $(RELEASE_TARGET)"

	$(PLATFORM_BREAKPAD_BINARY) libAnalytics_Comcast_debug.so > libAnalytics_Comcast.so.sym
	sed -i "1s/libAnalytics_Comcast_debug.so/libAnalytics_Comcast.so/" libAnalytics_Comcast.so.sym
	mv -f libAnalytics_Comcast.so.sym  $(PLATFORM_SYMBOL_PATH)
	echo "Debug symbol created for libAnalytics_Comcast.so"

%.o:%.cpp
	$(CXX) -c $< $(CFLAGS)  -o $@

clean:
	$(RM) -rf $(OBJ_IAV) $(OBJ_XCV) $(OBJ_XVISION) $(OBJ_XVINTER) *~ $(INSTPROGS) $(RELEASE_TARGET) $(DEBUG_TARGET)
	$(RM) -rf $(OBJ_IAV) $(OBJ_XCV) $(OBJ_XVISION) $(OBJ_XVINTER) $(OBJ_TH) *~ $(INSTPROGS) $(RELEASE_TARGET)

