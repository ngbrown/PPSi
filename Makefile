#
# Alessandro Rubini for CERN, 2011 -- public domain
#

# The default is not extension. Uncomment here or set on the command line
# PROTO_EXT ?= whiterabbit

# The default architecture is hosted GNU/Linux stuff
ARCH ?= unix

# Also, you can set USER_CFLAGS, like this (or on the command line)
# USER_CFLAGS = -DVERB_LOG_MSGS

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
	$(MAKE) -C pp_printf pp-printf.o
endif

# We need this -I so <arch/arch.h> can be found
CFLAGS += -Iarch-$(ARCH)/include

# proto-standard is always included, as it provides default function
# so the extension can avoid duplication of code.
ifdef PROTO_EXT
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
	$(LD) -Map $(TARGET).map1 -r -o $@ $(OBJ-y) $(PPSI_O_LDFLAGS) \
		--start-group $(LIBS) --end-group

# Finally, "make clean" is expected to work
clean:
	rm -f $$(find . -name '*.[oa]') *.bin $(TARGET) *~ $(TARGET).map*

# following targets from Makefile.kconfig
silentoldconfig:
	@mkdir -p include/config
	$(MAKE) -f Makefile.kconfig $@

scripts_basic config %config:
	$(MAKE) -f Makefile.kconfig $@

defconfig:
	@echo "Using unix_defconfig"
	@$(MAKE) -f Makefile.kconfig unix_defconfig

.config: silentoldconfig
