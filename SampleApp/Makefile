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

BUILD_PLATFORM := PLATFORM_RDKC

VA_ROOT_DIR  := ./..
BUILD_ROOT_DIR  := $(VA_ROOT_DIR)/..


# video-analytics Includes
INCPATH += $(VA_ROOT_DIR)/include


# Opensource Includes for RDKC
ifeq ($(BUILD_PLATFORM), PLATFORM_RDKC)
INCPATH += $(BUILD_ROOT_DIR)/plugins/
INCPATH += $(BUILD_ROOT_DIR)/plugins/soc/
INCPATH += $(BUILD_ROOT_DIR)/sdk/fsroot/src/amba/include/arch_s2l
INCPATH += $(BUILD_ROOT_DIR)/sdk/fsroot/src/amba/include
INCPATH += $(BUILD_ROOT_DIR)/opensource/include
INCPATH += $(BUILD_ROOT_DIR)/configMgr/include
INCPATH += $(BUILD_ROOT_DIR)/rdklogger/include
endif


# Linking path for RDKC
ifeq ($(BUILD_PLATFORM), PLATFORM_RDKC)
export PKG_CONFIG_PATH=../../opensource/lib/pkgconfig
LDFLAGS +=  -L$(VA_ROOT_DIR)/src -lvideoanalytics -Wl,-rpath-link=$(BUILD_ROOT_DIR)/configMgr/config/src
LDFLAGS +=  -L$(VA_ROOT_DIR)/src -lvideoanalytics -Wl,-rpath-link=$(BUILD_ROOT_DIR)/plugins/soc/
LDFLAGS += -L$(BUILD_ROOT_DIR)/rdklogger/src/.libs -lrdkloggers
LDFLAGS +=  -L$(VA_ROOT_DIR)/src -lvideoanalytics -Wl,-rpath-link=$(BUILD_ROOT_DIR)/opensource/lib
LDFLAGS += -ggdb `pkg-config --cflags --libs opencv`

endif


# Cross Compiler Path for RDKC
ifeq ($(BUILD_PLATFORM), PLATFORM_RDKC)
CXX = $(VA_ROOT_DIR)/../sdk/toolchain/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ 
endif

CFLAGS += $(addprefix -I, $(INCPATH))
CFLAGS += -fPIC -Wall 
LDFLAGS += -lpthread 

RM = rm -f  
TARGET_TEST := InItVA
TARGET_APP1 := Sample1
TARGET_APP2 := Sample2

all: $(TARGET_TEST) $(TARGET_APP1) $(TARGET_APP2)

TEST_SRCS = InitVAModule.cpp
APP_SRCS1 = Sample1.cpp
APP_SRCS2 = Sample2.cpp
TEST_OBJ = $(TEST_SRCS:.cpp=.o)
APP_OBJ1 = $(APP_SRCS1:.cpp=.o)
APP_OBJ2 = $(APP_SRCS2:.cpp=.o)

$(TARGET_TEST): $(TEST_OBJ)
	$(CXX) $^  $(LDFLAGS) -o $@

$(TARGET_APP1): $(APP_OBJ1)
	$(CXX) $^ -o $@ 

$(TARGET_APP2): $(APP_OBJ2)
	$(CXX) $^ -o $@

%.o:%.cpp
	$(CXX) -c $< $(CFLAGS) $(INCPATH) -o $@


clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.cpp=.o)	

.PHONY: all clean
