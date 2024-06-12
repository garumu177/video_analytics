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

INCLUDE_DIR := ./..
BUILD_ROOT_DIR  := $(RDK_PROJECT_ROOT_PATH)

export PKG_CONFIG_PATH=$(BUILD_ROOT_DIR)/opensource/lib/pkgconfig

#test-harness Includes
INCPATH += $(INCLUDE_DIR)/include

TARGET_LIB_TH = libtestharness.so

SRCS_TH = RdkCTestHarness.cpp RdkCTestHarnessPhaseII.cpp socket_client.cpp

# for JSON files
#CFLAGS += -DJSON_TEST_HARNESS_CHANGES
CFLAGS += -DTEST_HARNESS_SOCKET

INCPATH += $(BUILD_ROOT_DIR)/configMgr/include
INCPATH += $(BUILD_ROOT_DIR)/sdk/fsroot/src/amba/prebuild/third-party/armv7-a-hf/iva/include
INCPATH += $(BUILD_ROOT_DIR)/sdk/fsroot/src/amba/include/arch_s2l
INCPATH += $(BUILD_ROOT_DIR)/sdk/fsroot/src/amba/include
INCPATH += $(BUILD_ROOT_DIR)/opensource/include
INCPATH += $(BUILD_ROOT_DIR)/rdklogger/include
INCPATH += $(BUILD_ROOT_DIR)/video-analytics/include

LDFLAGS += -L$(BUILD_ROOT_DIR)/configMgr/config/src -lconfigmanager
LDFLAGS += -L$(BUILD_ROOT_DIR)/rdklogger/src/.libs -lrdkloggers -llog4c
#LDFLAGS += -L$(BUILD_ROOT_DIR)/opensource/lib -lcurl
LDFLAGS += `pkg-config --libs opencv`

CFLAGS += $(addprefix -I, $(INCPATH))
CFLAGS += `pkg-config --cflags opencv`
CFLAGS += -fPIC -Wall
CFLAGS += -std=c++11
##CXX = $(BUILD_ROOT_DIR)/sdk/toolchain/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -ggdb `pkg-config --cflags --libs opencv`

ifeq ($(ENABLE_OBJ_DETECTION), true)
CFLAGS += -D_OBJ_DETECTION_
endif

ifeq ($(XCAM_MODEL), SCHC2)
CFLAGS += -DXCAM2
endif

OBJS_TH = $(SRCS_TH:.cpp=.o)

all: $(TARGET_LIB_TH)

$(TARGET_LIB_TH): $(OBJS_TH)
	$(CXX) -shared -o $@ $^ $(LDFLAGS) -lrt

%.o:%.cpp
	$(CXX) -c $< $(CFLAGS)  -o $@


clean:
	-${RM} ${TARGET_LIB_TH} ${OBJS_TH}

.PHONY: all clean
