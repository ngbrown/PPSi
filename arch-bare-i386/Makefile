
CFLAGS += -ffreestanding -Os -fno-stack-protector -Itools
ARCH_LDFLAGS = -nostdlib -static -T $(ARCH).lds

# We need to undefine _FORTIFY_SOURCE, to prevent picking up external
# symbols in the inline versions of strcpy etc. This is documented in
# "man gcc" in the Ubuntu version.
CFLAGS += -U_FORTIFY_SOURCE

# All files are under A (short for ARCH) or L (short for lib): I'm lazy
A := arch-$(ARCH)
L := lib-bare

CFLAGS += -Ilib-bare

LIBARCH := $A/libarch.a

OBJ-libarch := $L/bare-startup.o \
	$L/main-loop.o \
	$L/bare-io.o \
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
# were not selected yet (e.g., pp_open_globals() ).
$(TARGET): $(TARGET).o $A/crt0.o $(LIBARCH)
	$(CC) -Wl,-Map,$(TARGET).map2 $(ARCH_LDFLAGS) -o $@ $A/crt0.o \
		$(TARGET).o -L$A -larch -L$D -lstd

