
# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

CFLAGS += -Itools

OBJ-y += $A/sim-startup.o \
	$A/main-loop.o \
	$A/sim-io.o \
	lib/div64.o
#	lib/dump-funcs.o \
#	lib/conf.o \
#	lib/libc-functions.o \
#	lib/cmdline.o \

# Support only "sim" time operations
TIME := sim
include time-$(TIME)/Makefile

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_globals() ).
$(TARGET): $(TARGET).o
	$(CC) -Wl,-Map,$(TARGET).map2 -o $@ $(TARGET).o -lrt
