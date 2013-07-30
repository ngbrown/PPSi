
# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

CFLAGS += -DCONFIG_PPSI_RUNTIME_VERBOSITY -Itools


LIBARCH := $A/libarch.a

OBJ-libarch := $A/unix-startup.o \
	$A/main-loop.o \
	$A/unix-io.o \
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

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_globals() ).
$(TARGET): $(TARGET).o $(LIBARCH)
	$(CC) -Wl,-Map,$(TARGET).map2 -o $@ $(TARGET).o \
		-L$A -larch -L$D -lstd -lrt

