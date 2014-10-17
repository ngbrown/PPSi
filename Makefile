#
# Alessandro Rubini for CERN, 2011,2013 -- public domain
#

# We are now Kconfig-based
-include $(CURDIR)/.config

# We still accept command-line choices like we used to do.
# Also, we must remove the quotes from these Kconfig values
PROTO_EXT ?= $(patsubst "%",%,$(CONFIG_EXTENSION))
ARCH ?= $(patsubst "%",%,$(CONFIG_ARCH))
CROSS_COMPILE ?= $(patsubst "%",%,$(CONFIG_CROSS_COMPILE))
WRPCSW_ROOT ?= $(patsubst "%",%,$(CONFIG_WRPCSW_ROOT))

# For "make config" to work, we need a valid ARCH
ifeq ($(ARCH),)
   ARCH = unix
endif

#### In theory, users should not change stuff below this line (but please read)

# classic cross-compilation tool-set
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

# To cross-build bare stuff (x86-64 under i386 and vice versa). We need this:
LD := $(LD) $(shell echo $(CONFIG_ARCH_LDFLAGS))
CC := $(CC) $(shell echo $(CONFIG_ARCH_CFLAGS))
# (I apologize, but I couldn't find another way to remove quotes)

# Instead of repeating "ppsi" over and over, bless it TARGET
TARGET = ppsi

# we should use linux/scripts/setlocalversion instead...
VERSION = $(shell git describe --always --dirty)

# The main target is the big object file.
all: $(TARGET).o

# CFLAGS to use. Both this Makefile (later) and app-makefile may grow CFLAGS
CFLAGS = $(USER_CFLAGS)
CFLAGS += -Wall -O2 -ggdb -Iinclude -fno-common
CFLAGS += -DPPSI_VERSION=\"$(VERSION)\"

# to avoid ifdef as much as possible, I use the kernel trick for OBJ variables
OBJ-y := fsm.o diag.o

# Include arch code. Each arch chooses its own time directory..
include arch-$(ARCH)/Makefile

# include pp_printf code, by default the "full" version. Please
# set CONFIG_PRINTF_NONE or CONFIG_PRINTF_XINT if needed.
ifndef CONFIG_NO_PRINTF
OBJ-y += pp_printf/pp-printf.o

pp_printf/pp-printf.o: $(wildcard pp_printf/*.[ch])
	$(MAKE) -C pp_printf pp-printf.o CC="$(CC)" LD="$(LD)"
endif

# We need this -I so <arch/arch.h> can be found
CFLAGS += -Iarch-$(ARCH)/include

# proto-standard is always included, as it provides default function
# so the extension can avoid duplication of code.
ifneq ($(PROTO_EXT),)
  include proto-ext-$(PROTO_EXT)/Makefile
endif
include proto-standard/Makefile

# ...and the TIME choice sets the default operations
CFLAGS += -DDEFAULT_TIME_OPS=$(TIME)_time_ops
CFLAGS += -DDEFAULT_NET_OPS=$(TIME)_net_ops

export CFLAGS

# And this is the rule to build our target.o file. The architecture may
# build more stuff. Please note that ./MAKEALL looks for $(TARGET)
# (i.e., the ELF which is either the  output or the input to objcopy -O binary)
#
# The object only depends on OBJ-y because each subdirs added needed
# libraries: see proto-standard/Makefile as an example.

$(TARGET).o: silentoldconfig $(OBJ-y)
	$(LD) -Map $(TARGET).map1 -r -o $@ $(PPSI_O_LDFLAGS) \
		--start-group $(OBJ-y) --end-group

$(OBJ-y): $(wildcard include/ppsi/*.h)

# Finally, "make clean" is expected to work
clean:
	rm -f $$(find . -name '*.[oa]' ! -path './scripts/kconfig/*') *.bin $(TARGET) *~ $(TARGET).map*

# following targets from Makefile.kconfig
silentoldconfig:
	@mkdir -p include/config
	$(MAKE) -f Makefile.kconfig $@

%config:
	$(MAKE) -f Makefile.kconfig $@

defconfig:
	@echo "Using unix_defconfig"
	@$(MAKE) -f Makefile.kconfig unix_defconfig

.config: silentoldconfig
