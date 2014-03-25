
CFLAGS += -ffreestanding -Os \
	-ffunction-sections -fdata-sections \
	-mmultiply-enabled -mbarrel-shift-enabled \
	-Itools

# Root of wrpc-sw project
WRPCSW_ROOT ?= $(CONFIG_WRPCSW_ROOT)

CFLAGS += -I$(WRPCSW_ROOT)/include -I$(WRPCSW_ROOT)/include/std -I$(WRPCSW_ROOT)/softpll

PPSI_O_LDFLAGS = -u wrc_ptp_init

# Let's use the pp_printf we already have in wrpc-sw
CONFIG_NO_PRINTF = y

# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

OBJ-y += \
	$A/wrpc-io.o \
	$A/wrpc-spll.o \
	$A/wrpc-calibration.o \
	$A/wrc_ptp_ppsi.o \
	lib/dump-funcs.o \
	lib/div64.o

# We only support "wrpc" time operations
TIME := wrpc
include time-wrpc/Makefile

$(TARGET).o: $(LIBARCH)

$(OBJ-y): wrpcsw_root_check

wrpcsw_root_check:
	if ! [ -e "$(WRPCSW_ROOT)/include/wrc.h" ]; then\
		echo "Error: WRPCSW_ROOT: can't find $(WRPCSW_ROOT)/include/wrc.h" >&2;\
		exit 1;\
	fi
