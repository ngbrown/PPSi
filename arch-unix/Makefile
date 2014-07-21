
# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

CFLAGS += -Itools

OBJ-y += $A/unix-startup.o \
	$A/main-loop.o \
	$A/unix-io.o \
	$A/unix-conf.o \
	lib/cmdline.o \
	lib/conf.o \
	lib/libc-functions.o \
	lib/dump-funcs.o \
	lib/drop.o \
	lib/div64.o

# The user can set TIME=, but we pick unix time by default
TIME ?= unix

include time-$(TIME)/Makefile

# Unix time operations are always included as a fallback
ifneq ($(TIME),unix)
include time-unix/Makefile
endif
CFLAGS += -Itime-unix

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_init_globals() ).
$(TARGET): $(TARGET).o
	$(CC) -Wl,-Map,$(TARGET).map2 -o $@ $(TARGET).o -lrt

