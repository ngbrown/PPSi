# Alessandro Rubini for CERN, 2011 -- public domain

CFLAGS += -ffreestanding -Os \
	-ffunction-sections -fdata-sections \
	-mmultiply-enabled -mbarrel-shift-enabled \
	-Itools

# Root of wrpc-sw project
WRPCSW_ROOT ?= ..

CFLAGS += -I$(WRPCSW_ROOT)/include -I$(WRPCSW_ROOT)/softpll \
	  -I$(WRPCSW_ROOT)/boards/spec \

# FIXME ptp-noposix root; used temporarily until we clean up
PTPNOPOSIX_ROOT = $(WRPCSW_ROOT)/ptp-noposix

CFLAGS += -I$(PTPNOPOSIX_ROOT)/PTPWRd/dep -I$(PTPNOPOSIX_ROOT)/PTPWRd

# Let's use the pp_printf we already have in wrpc-sw
CONFIG_NO_PRINTF = y

# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

LIBARCH := $A/libarch.a

LIBS += -L$A -larch


OBJ-libarch := \
	$A/wrpc-io.o \
	$A/wrpc-spll.o \
	$A/wrpc-calibration.o \
	$A/wrc_ptp_ppsi.o \
	lib/dump-funcs.o \
	lib/div64.o

# We only support "wrpc" time operations
TIME := wrpc
include time-wrpc/Makefile

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

$(TARGET).o: $(LIBARCH)
