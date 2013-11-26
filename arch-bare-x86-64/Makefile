
CFLAGS += -ffreestanding -Os -fno-stack-protector -Itools
ARCH_LDFLAGS = -nostdlib -static -T $A/$(ARCH).lds

# We need to undefine _FORTIFY_SOURCE, to prevent picking up external
# symbols in the inline versions of strcpy etc. This is documented in
# "man gcc" in the Ubuntu version.
CFLAGS += -U_FORTIFY_SOURCE

# All files are under A (short for ARCH) or L (short for lib): I'm lazy
A := arch-$(ARCH)
L := lib-bare

CFLAGS += -Ilib-bare

OBJ-y += $A/crt0.o \
	$L/bare-startup.o \
	$L/main-loop.o \
	$L/bare-io.o \
	$A/syscall.o \
	$A/syscalls.o \
	lib/libc-functions.o \
	lib/dump-funcs.o \
	lib/cmdline.o \
	lib/div64.o

# We only support "bare" time operations
TIME := bare
include time-bare/Makefile

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

all: $(TARGET)

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_init_globals() ).
$(TARGET): $(TARGET).o
	$(CC) -Wl,-Map,$(TARGET).map2 $(ARCH_LDFLAGS) -o $@ $(TARGET).o

