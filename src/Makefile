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

# include ${RDK_SOURCE_PATH}/AppsRule.mak

export PKG_CONFIG_PATH=$(RDK_PROJECT_ROOT_PATH)/opensource/lib/pkgconfig

CFLAGS  += -I${RDK_PROJECT_ROOT_PATH}/rdklogger/include
LIBFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/rdklogger/src/.libs/ -lrdkloggers
LIBFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/opensource/lib -llog4c

#video-analytics Includes
INCPATH += $(RDK_SOURCE_PATH)/include
INCPATH += $(RDK_SOURCE_PATH)/XCV
INCPATH += $(RDK_SOURCE_PATH)/XCV/thirdparty
INCPATH += $(RDK_SOURCE_PATH)/XCV/components/objValidation
INCPATH += $(RDK_SOURCE_PATH)/XCV/components/utils
INCPATH += $(RDK_PROJECT_ROOT_PATH)/opensource/include
INCPATH += $(RDK_PROJECT_ROOT_PATH)/configMgr/include
INCPATH += $(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/amba/prebuild/third-party/armv7-a-hf/iva/include

CFLAGS += $(addprefix -I, $(INCPATH))
CFLAGS += -g -fPIC -Wall
ifeq ($(XCAM_MODEL), XHB1)
CFLAGS += -O3  -march=armv8-a -ftree-vectorize
CFLAGS += -DDBC
else
CFLAGS += -O3 -mcpu=cortex-a9 -ftree-vectorize -mfloat-abi=hard -mfpu=neon
endif

ifeq ($(XCAM_MODEL), SCHC2)
CFLAGS += -DXCAM2
endif

ifeq ($(XCAM_MODEL), XHC3)
CFLAGS += -DXCAM3
endif

CFLAGS += `pkg-config --cflags opencv`
CFLAGS += -D_ROI_ENABLED_

ifeq ($(ENABLE_OBJ_DETECTION), true)
CFLAGS += -D_OBJ_DETECTION_
endif


LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/configMgr/config/src -lconfigmanager
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/video-analytics/XCV/build -lbgs
LDFLAGS += `pkg-config --libs opencv`
LDFLAGS += -lpthread

RM = rm -f
RELEASE_TARGET = libvideoanalytics.so
DEBUG_TARGET = libvideoanalytics_debug.so

SRCS_VA = RdkCVAManager.cpp RdkCVideoAnalytics.cpp

OBJS_VA = $(SRCS_VA:.cpp=.o)

all: $(RELEASE_TARGET) $(DEBUG_TARGET) install

$(RELEASE_TARGET): $(OBJS_VA)
	$(CXX) -shared $(LDFLAGS) $^ -o $@ -lrt
	$(STRIP) $(RELEASE_TARGET)

$(DEBUG_TARGET): $(OBJS_VA)
	$(CXX) -shared $(LDFLAGS) $^ -o $@ -lrt

%.o:%.cpp
	$(CXX) -std=c++11 -c $< $(CFLAGS)  -o $@

install:
	$(PLATFORM_BREAKPAD_BINARY) $(DEBUG_TARGET) > $(RELEASE_TARGET).sym
	sed -i "1s/$(DEBUG_TARGET)/$(RELEASE_TARGET)/" $(RELEASE_TARGET).sym
	mv -f $(RELEASE_TARGET).sym  $(PLATFORM_SYMBOL_PATH)
	echo "Debug symbol created for $(RELEASE_TARGET)"

clean:
	-${RM} ${RELEASE_TARGET} ${DEBUG_TARGET} ${OBJS_VA}

.PHONY: all clean


