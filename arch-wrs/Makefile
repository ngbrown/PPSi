# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

CFLAGS += -Itools

OBJ-y += $A/wrs-startup.o \
	$A/main-loop.o \
	$A/wrs-io.o \
	$A/wrs-calibration.o \
	$A/wrs-ipcserver.o \
	$A/shmem.o \
	arch-unix/unix-conf.o \
	lib/cmdline.o \
	lib/conf.o \
	lib/libc-functions.o \
	lib/dump-funcs.o \
	lib/drop.o \
	lib/div64.o

# The user can set TIME=, but wrs is default
TIME ?= wrs

include time-$(TIME)/Makefile

# Unix time operations are always included as a fallback
include time-unix/Makefile
CFLAGS += -Iproto-ext-whiterabbit

# mini-rpc directory contains minipc library
export CROSS_COMPILE
MINIPC_DIR := $A/mini-rpc
MINIPC_LIB := $(MINIPC_DIR)/libminipc.a
CFLAGS += -I$(MINIPC_DIR)
.PHONY: $(MINIPC_LIB)
$(MINIPC_LIB):
	$(MAKE) -C $(MINIPC_DIR)

OBJ-y += $(MINIPC_LIB)

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_init_globals() ).
$(TARGET): $(TARGET).o
	$(CC) -Wl,-Map,$(TARGET).map2 -o $@ $(TARGET).o -lrt

