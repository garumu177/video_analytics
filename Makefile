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

# To build VA with RDKC_VA_with_PLUGIN, BUILD_PLATFORM should be enabled
#BUILD_PLATFORM := PLATFORM_RDKC

#To enable VA for Test Harness, SUPPORT_TEST_HARNESS should be enabled
SUPPORT_TEST_HARNESS := yes

#To enable snapshoter imagetool
BUILD_IMAGETOOLS := yes
modules := src

ifeq ($(BUILD_PLATFORM), PLATFORM_RDKC)
modules += SampleApp
endif

ifeq ($(SUPPORT_TEST_HARNESS), yes)
modules += test-harness
endif

ifeq ($(SUPPORT_XCV_PROCESS), yes)
#ifneq ($(XCAM_MODEL), SCHC2)
modules += xvision
#endif
endif

ifeq ($(BUILD_IMAGETOOLS), yes)
modules += imagetools
endif

all:
	@for m in $(modules); do echo $$m; make -C $$m $@ || exit 1; done

clean:
	@for m in $(modules); do echo $$m; make -C $$m $@ || exit 1; done

install:
	@for m in $(modules); do echo $$m; make -C $$m $@ || exit 1; done

.PHONY: all clean install
