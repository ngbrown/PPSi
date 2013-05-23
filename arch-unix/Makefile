# Alessandro Rubini for CERN, 2011 -- public domain

# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

CFLAGS += -DCONFIG_PPSI_RUNTIME_VERBOSITY -Itools


LIBARCH := $A/libarch.a

OBJ-libarch := $A/unix-startup.o \
	$A/main-loop.o \
	$A/unix-socket.o \
	$A/unix-io.o \
	$A/unix-time.o \
	lib/cmdline.o \
	lib/conf.o \
	lib/libc-functions.o \
	lib/dump-funcs.o \
	lib/div64.o

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_globals() ).
$(TARGET): $(TARGET).o $(LIBARCH)
	$(CC) -Wl,-Map,$(TARGET).map2 -o $@ $(TARGET).o \
		-L$A -larch -L$D -lstd -lrt

