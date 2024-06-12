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

#SUPPORT_MXML = yes
#USE_LIBXML := yes
include ${RDK_SOURCE_PATH}/AppsRule.mak
export PKG_CONFIG_PATH=$(RDK_PROJECT_ROOT_PATH)/opensource/lib/pkgconfig

ENABLE_DIRECT_FRAME_READ := true

#Includes
INCPATH += $(RDK_SOURCE_PATH)/include
INCPATH += $(RDK_PROJECT_ROOT_PATH)/opensource/include
INCPATH += $(RDK_PROJECT_ROOT_PATH)/configMgr/include
INCPATH  += -I$(RDK_PROJECT_ROOT_PATH)/utility/misc/include
INCPATH += ${RDK_PROJECT_ROOT_PATH}/rdklogger/include

ifeq ($(ENABLE_XSTREAMER), true)
$(info( '*************XSTREAMER ENABLED**************')
CFLAGS   += -D_HAS_XSTREAM_
INCPATH  += $(RDK_PROJECT_ROOT_PATH)/camera-hal/streamer
INCPATH  += $(RDK_PROJECT_ROOT_PATH)/camera-hal/streamer/xBroker
INCPATH  += $(RDK_PROJECT_ROOT_PATH)/camera-hal/streamer/xBroker/xConsumer

LDFLAGS  = -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/vendor/img/fs/shadow_root/usr/lib -lstreamerconsumer

ifeq ($(ENABLE_DIRECT_FRAME_READ), true)
$(info( '*************DIRECT READ FRAME ENABLED**************')
CFLAGS   += -D_DIRECT_FRAME_READ_
LDFLAGS  += -Wl,-rpath-link=$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/src/vendor/img/fs/shadow_root/usr/lib
endif

#LDFLAGS  += -L$(RDK_PROJECT_ROOT_PATH)/opensource/lib -lcurl -lz
LDFLAGS  += -Wl,-rpath-link=$(RDK_PROJECT_ROOT_PATH)/opensource/lib
else
INCPATH += $(RDK_PROJECT_ROOT_PATH)/plugins
INCPATH += $(RDK_PROJECT_ROOT_PATH)/plugins/soc
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/plugins/soc -lhalrtc
endif

# Xcam2
ifeq ($(XCAM_MODEL), SCHC2)
CFLAGS  += -DXCAM2
endif

# Doorbell Camera
ifeq ($(XCAM_MODEL), XHB1)
CFLAGS += -DS5L
endif

## XHC3
ifeq ($(XCAM_MODEL), XHC3)
CFLAGS += -DS2L35M
endif

CFLAGS += $(addprefix -I, $(INCPATH))
CFLAGS += -g -fPIC -Wall
CFLAGS += `pkg-config --cflags opencv`
CFLAGS  += "-lrt"
CFLAGS += -DBREAKPAD
CFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/breakpadwrap/

LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/breakpadwrap -lbreakpadwrap
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/configMgr/config/src -lconfigmanager
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/rdklogger/src/.libs/ -lrdkloggers
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/opensource/lib -llog4c
LDFLAGS += -L${PROJ_ROOT}/vendor/img/fs/shadow_root/usr/lib -lmxml
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/utility/misc/src/ -lmiscutils
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/video-analytics/imagetools/
ifeq ($(XCAM_MODEL), XHC3)
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/sdk/ -Wl,--no-as-needed -ldl -lpthread -Wl,-rpath-link=$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/ramdisk/usr/lib -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/ramdisk/usr/lib -L$(RDK_PROJECT_ROOT_PATH)/sdk/toolchain/linaro-armv7ahf-2019.02-gcc7.4/arm-linux-gnueabihf/libc/usr/lib -L$(RDK_PROJECT_ROOT_PATH)/utility/RFCConfig/src -lrfcconfig -pthread -fpermissive -fPIC
else
LDFLAGS += -L$(RDK_PROJECT_ROOT_PATH)/sdk/ -Wl,--no-as-needed -ldl -lpthread -Wl,-rpath-link=$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/ramdisk/usr/lib -L$(RDK_PROJECT_ROOT_PATH)/sdk/fsroot/ramdisk/usr/lib -L$(RDK_PROJECT_ROOT_PATH)/sdk/toolchain/linaro-armv7ahf-2014.12-gcc4.9/arm-linux-gnueabihf/libc/usr/lib -L$(RDK_PROJECT_ROOT_PATH)/utility/RFCConfig/src -lrfcconfig -pthread -fpermissive -fPIC
endif

LDFLAGS += `pkg-config --libs opencv`
LDFLAGS += -lpthread
LDFLAGS += -Wl,-unresolved-symbols=ignore-in-shared-libs

RM = rm -f

RELEASE_TARGET = libimagetools.so
DEBUG_TARGET = libimagetools_debug.so
TARGET =rdkc_snapshooter 
SRC = RdkCVAImageTools.cpp
TARGET_SRC=main.cpp 
RELEASE_LIB_OBJS = $(RELEASE_LIB_SRC:.cpp=.o)
OBJS_IMGTOOLS = $(SRC:.cpp=.o)
OBJ_TARGET = $(TARGET_SRC:.cpp=.o)
all: $(RELEASE_TARGET) $(DEBUG_TARGET) $(TARGET)install

$(RELEASE_TARGET): $(OBJS_IMGTOOLS)
	$(CXX) $(LDFLAGS) $(CFLAGS) $^ -shared -o $(@)
	$(STRIP) $(RELEASE_TARGET)

$(DEBUG_TARGET): $(OBJS_IMGTOOLS)
	$(CXX) $(LDFLAGS) $(CFLAGS) $^ -shared -o $(@)

$(TARGET): $(OBJ_TARGET)
	$(CXX) $(CFLAGS) -o $(@) $^ $(LIBS) $(LDFLAGS) -limagetools
	$(STRIP) $(TARGET)
%.o:%.cpp
	$(CXX) -c $< $(CFLAGS)  -o $@

install:
	$(PLATFORM_BREAKPAD_BINARY) $(DEBUG_TARGET) > $(RELEASE_TARGET).sym
	sed -i "1s/$(DEBUG_TARGET)/$(RELEASE_TARGET)/" $(RELEASE_TARGET).sym
	mv -f $(RELEASE_TARGET).sym  $(PLATFORM_SYMBOL_PATH)
	echo "Debug symbol created for $(RELEASE_TARGET)"

clean:
	-${RM} ${RELEASE_TARGET} $(DEBUG_TARGET) ${OBJS_IMGTOOLS}

.PHONY: all clean

