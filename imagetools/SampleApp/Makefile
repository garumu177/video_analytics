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

RDK_PROJECT_ROOT_PATH = ../../../
TARGET = Snapshot
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)
export PKG_CONFIG_PATH=$(RDK_PROJECT_ROOT_PATH)/opensource/lib/pkgconfig

CFLAGS  += -I${RDK_PROJECT_ROOT_PATH}/rdklogger/include
LIBFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/rdklogger/src/.libs/ -lrdkloggers
LIBFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/opensource/lib -llog4c

CFLAGS += -I../
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/sysapps/apps/cgi/librdkcstream
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/sysapps/apps/cgi/libcgicomm/inc
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/sysapps/apps/hydra/include
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/share/libsccomm/inc
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/share/sc_config
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/video-analytics/include
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/amba/prebuild/third-party/armv7-a-hf/iva/include
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/opensource/include
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/configMgr/include
CFLAGS += -fPIC -Wall
CFLAGS += `pkg-config --cflags opencv`

ifeq ($(XCAM_MODEL), SCHC2)
LIBFLAGS += -limg_algo_s3l
else
LIBFLAGS += -limg_algo_s2l 
endif

LIBFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/amba/prebuild/imgproc/img_lib/ -L$(RDK_PROJECT_ROOT_PATH)/configMgr/config/src -lconfigmanager -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src//share/libsccomm/bin -lsccomm -lpthread  -lresolv -lrt -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src//amba/prebuild/third-party/armv7-a-hf/alsa-lib/usr/lib -lasound -lm -ldl -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/vendor/img/fs/shadow_root/usr/lib -lmxml -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src//../../../opensource/lib -lssl -lcrypto -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/vendor/img/fs/shadow_root/usr/lib -lcgicomm -lpthread -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/vendor/img/fs/shadow_root/usr/lib -lrdkcstream

#LIBFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/sysapps/apps/cgi/librdkc/bin -lrdkc
LIBFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/vendor/img/fs/shadow_root/usr/lib -lrdkcstream
LIBFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/configMgr/config/src -lconfigmanager
LIBFLAGS += `pkg-config --libs opencv`
LIBFLAGS += -L../ -limagetools

RM = rm -f
all: $(TARGET)

$(TARGET):$(OBJS)
	$(CXX) $(LIBFLAGS) $^ -o $@

%.o:%.cpp
	$(CXX) -c $< $(CFLAGS) -o $@

clean:
	-${RM} ${TARGET} ${OBJS}

.PHONY: all clean
