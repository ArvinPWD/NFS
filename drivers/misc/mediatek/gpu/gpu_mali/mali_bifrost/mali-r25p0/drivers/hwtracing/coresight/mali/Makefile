# SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#
# (C) COPYRIGHT 2022-2023 ARM Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the
# GNU General Public License version 2 as published by the Free Software
# Foundation, and any use by you of this program is subject to the terms
# of such GNU license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you can access it online at
# http://www.gnu.org/licenses/gpl-2.0.html.
#
#

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
KDIR ?= $(KERNEL_SRC)
M ?= $(shell pwd)

ifeq ($(KDIR),)
    $(error Must specify KDIR to point to the kernel to target))
endif

CONFIGS :=

ifeq ($(MALI_KCONFIG_EXT_PREFIX),)
    CONFIG_MALI_CORESIGHT ?= n
    ifeq ($(CONFIG_MALI_CORESIGHT),y)

        ifeq ($(CONFIG_ARM64), y)
            CONFIG_CORESIGHT_MALI_SOURCES_ETM ?= y
        endif

        CONFIG_CORESIGHT_MALI_SOURCES_ITM ?= y
        CONFIG_CORESIGHT_MALI_SOURCES_ELA ?= y
    endif

    CONFIGS += \
        CONFIG_MALI_CORESIGHT \
        CONFIG_CORESIGHT_MALI_SOURCES_ETM \
        CONFIG_CORESIGHT_MALI_SOURCES_ITM \
        CONFIG_CORESIGHT_MALI_SOURCES_ELA

endif

#
# MAKE_ARGS to pass the custom CONFIGs on out-of-tree build
#
# Generate the list of CONFIGs and values.
# $(value config) is the name of the CONFIG option.
# $(value $(value config)) is its value (y, m).
# When the CONFIG is not set to y or m, it defaults to n.
MAKE_ARGS := $(foreach config,$(CONFIGS), \
                    $(if $(filter y m,$(value $(value config))), \
                        $(value config)=$(value $(value config)), \
                        $(value config)=n))

#
# EXTRA_CFLAGS to define the custom CONFIGs on out-of-tree build
#
# Generate the list of CONFIGs defines with values from CONFIGS.
# $(value config) is the name of the CONFIG option.
# When set to y or m, the CONFIG gets defined to 1.
EXTRA_CFLAGS := $(foreach config,$(CONFIGS), \
                    $(if $(filter y m,$(value $(value config))), \
                        -D$(value config)=1))

#
# KBUILD_EXTRA_SYMBOLS to prevent warnings about unknown functions
#
GPU_SYMBOLS = $(M)/../../../gpu/arm/Module.symvers
ifeq ($(MALI_KCONFIG_EXT_PREFIX),)
    GPU_SYMBOLS = $(M)/../../../gpu/arm/midgard/Module.symvers
endif

EXTRA_SYMBOLS += \
    $(GPU_SYMBOLS)


# The following were added to align with W=1 in scripts/Makefile.extrawarn
# from the Linux source tree
KBUILD_CFLAGS += -Wall -Werror
KBUILD_CFLAGS += -Wextra -Wunused -Wno-unused-parameter
KBUILD_CFLAGS += -Wmissing-declarations
KBUILD_CFLAGS += -Wmissing-format-attribute
KBUILD_CFLAGS += -Wmissing-prototypes
KBUILD_CFLAGS += -Wold-style-definition
KBUILD_CFLAGS += -Wmissing-include-dirs
KBUILD_CFLAGS += $(call cc-option, -Wunused-but-set-variable)
KBUILD_CFLAGS += $(call cc-option, -Wunused-const-variable)
KBUILD_CFLAGS += $(call cc-option, -Wpacked-not-aligned)
KBUILD_CFLAGS += $(call cc-option, -Wstringop-truncation)
# The following turn off the warnings enabled by -Wextra
KBUILD_CFLAGS += -Wno-missing-field-initializers
KBUILD_CFLAGS += -Wno-sign-compare
KBUILD_CFLAGS += -Wno-type-limits
# The following ensures the stack frame does not get larger than a page
KBUILD_CFLAGS += -Wframe-larger-than=4096

KBUILD_CPPFLAGS += -DKBUILD_EXTRA_WARN1

all:
	$(MAKE) -C $(KDIR) M=$(M) $(MAKE_ARGS) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" modules

modules_install:
	$(MAKE) -C $(KDIR) M=$(M) $(MAKE_ARGS) modules_install

clean:
	$(MAKE) -C $(KDIR) M=$(M) $(MAKE_ARGS) clean
