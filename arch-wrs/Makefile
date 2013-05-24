# Alessandro Rubini for CERN, 2011 -- public domain

# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

CFLAGS += -DCONFIG_PPSI_RUNTIME_VERBOSITY -Itools


LIBARCH := $A/libarch.a

OBJ-libarch := $A/wrs-startup.o \
	$A/main-loop.o \
	$A/wrs-io.o \
	$A/wrs-calibration.o \
	lib/cmdline.o \
	lib/conf.o \
	lib/libc-functions.o \
	lib/dump-funcs.o \
	lib/div64.o

# The user can set TIME=, but we pick unix time by default
TIME ?= unix

include time-$(TIME)/Makefile

# Unix time operations are always included as a fallback
include time-unix/Makefile
CFLAGS += -Itime-unix

# mini-rpc directory contains minipc library
MINIPC_DIR := $A/mini-rpc
MINIPC_LIB := $(MINIPC_DIR)/libminipc.a
.PHONY: $(MINIPC_LIB)
$(MINIPC_LIB):
	make -C $(MINIPC_DIR)

$(LIBARCH): $(OBJ-libarch) $(MINIPC_LIB)
	$(AR) r $@ $^

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_globals() ).
$(TARGET): $(TARGET).o $(LIBARCH)
	$(CC) -Wl,-Map,$(TARGET).map2 -o $@ $(TARGET).o \
		-L$A -larch -L$D -L$(MINIPC_DIR) -lminipc -lstd -lrt

