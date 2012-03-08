# Alessandro Rubini for CERN, 2011 -- public domain

# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

LIBARCH := $A/libarch.a

OBJ-libarch := $A/posix-startup.o \
	$A/main-loop.o \
	$A/posix-socket.o \
	$A/posix-io.o \
	$A/posix-timer.o \
	lib/cmdline.o \
	lib/div64.o

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_instance() ).
$(TARGET): $(TARGET).o $(LIBARCH)
	$(CC) -Wl,-Map,$(TARGET).map2 -o $@ $(TARGET).o \
		-L$A -larch -L$D -lstd -lrt

