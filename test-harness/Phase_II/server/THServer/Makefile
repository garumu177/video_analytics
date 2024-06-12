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

TH_DIR := ./..
TH_ROOT_DIR  := ./../..

#test-harness Includes
INCPATH += $(TH_DIR)/includes
INCPATH += $(TH_DIR)/SocketServer

TARGET_LIB_TH = THServer

SRCS_TH = TestHarnessServer.cpp THServerMain.cpp

LDFLAGS += -L$(TH_DIR)/SocketServer -lsocketserver
LDFLAGS += -L$(TH_ROOT_DIR)/opensource/lib -lcJSON
LDFLAGS += -lpthread

CFLAGS += `pkg-config --cflags opencv`
CFLAGS += $(addprefix -I, $(INCPATH))
CFLAGS += -fPIC -Wall

CXX = g++ -std=c++0x

LDFLAGS += `pkg-config --libs opencv`

OBJS_TH = $(SRCS_TH:.cpp=.o)

all: $(TARGET_LIB_TH)

$(TARGET_LIB_TH): $(OBJS_TH)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o:%.cpp
	$(CXX) -c $< $(CFLAGS)  -o $@

clean:
	${RM} ${TARGET_LIB_TH} ${OBJS_TH}

.PHONY: all clean
