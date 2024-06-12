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

BUILD_ROOT_DIR := $(RDK_PROJECT_ROOT_PATH)

export PKG_CONFIG_PATH=$(BUILD_ROOT_DIR)/opensource/lib/pkgconfig

TARGET_LIB_TH = TestHarness_x86 

SRCS_TH = main_iva_x86.cpp

CFLAGS += -DTEST_HARNESS_SOCKET

INCPATH += $(BUILD_ROOT_DIR)/configMgr/include
INCPATH += $(BUILD_ROOT_DIR)/opensource/include
INCPATH += $(BUILD_ROOT_DIR)/video-analytics/include
INCPATH += $(BUILD_ROOT_DIR)/video-analytics/test-harness/Phase_II/include/

LDFLAGS += -L$(BUILD_ROOT_DIR)/configMgr/config/src -lconfigmanager
LDFLAGS += -L$(BUILD_ROOT_DIR)/video-analytics/src -lvideoanalytics
LDFLAGS += -L$(BUILD_ROOT_DIR)/video-analytics/test-harness/Phase_II/src -ltestharness
LDFLAGS += -L$(BUILD_ROOT_DIR)/video-analytics/xcv/build -lbgs

LDFLAGS += `pkg-config --libs opencv`

CFLAGS += $(addprefix -I, $(INCPATH))
CFLAGS += `pkg-config --cflags opencv`
CFLAGS += -fPIC -Wall
CFLAGS += -std=c++11
##CXX = $(BUILD_ROOT_DIR)/sdk/toolchain/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -ggdb `pkg-config --cflags --libs opencv`

OBJS_TH = $(SRCS_TH:.cpp=.o)

all: $(TARGET_LIB_TH)

$(TARGET_LIB_TH): $(OBJS_TH)
	$(CXX) -o $@ $^ $(LDFLAGS) -lrt

%.o:%.cpp
	$(CXX) -c $< $(CFLAGS)  -o $@


clean:
	-${RM} ${TARGET_LIB_TH} ${OBJS_TH}

.PHONY: all clean
