# Alessandro Rubini for CERN, 2011 -- public domain

CFLAGS += -ffreestanding -Os -fno-stack-protector
ARCH_LDFLAGS = -nostdlib -static -T $(ARCH).lds

# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

LIBARCH := $A/libarch.a

OBJ-libarch := $A/bare-startup.o \
	$A/main-loop.o \
	$A/bare-socket.o \
	$A/bare-timeout.o \
	$A/bare-io.o \
	$A/bare-time.o \
	$A/syscall.o \
	$A/syscalls.o \
	lib/libc-functions.o \
	lib/cmdline.o \
	lib/div64.o

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_instance() ).
$(TARGET): $(TARGET).o $A/crt0.o $(LIBARCH)
	$(CC) -Wl,-Map,$(TARGET).map2 $(ARCH_LDFLAGS) -o $@ $A/crt0.o \
		$(TARGET).o -L$A -larch -L$D -lstd

