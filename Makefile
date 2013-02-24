#
# Alessandro Rubini for CERN, 2011 -- public domain
#

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

# The main target is the big object file.
all: $(TARGET).o

# CFLAGS to use. Both this Makefile (later) and app-makefile may grow CFLAGS
CFLAGS += -Wall -O2 -ggdb -Iinclude

# to avoid ifdef as much as possible, I use the kernel trick for OBJ variables
OBJ-y := fsm.o

# include pp_printf code, by default the "full" version. Please
# set CONFIG_PRINTF_NONE or CONFIG_PRINTF_XINT if needed.
OBJ-y += pp_printf/pp-printf.o

pp_printf/pp-printf.o: $(wildcard pp_printf/*.[ch])
	$(MAKE) -C pp_printf pp-printf.o

include diag/Makefile

# Update 2012-07-10
# proto-standard should always be included, but in this current devel step
# each proto is intended as a separate plugin (not properly an "extension").
# See below (ifdef PROTO_EXT/else)
#include proto-standard/Makefile

# including the extension Makefile, if an extension is there
# Uncomment the following line for whiterabbit extension
# PROTO_EXT ?= whiterabbit
ifdef PROTO_EXT
  include proto-ext-$(PROTO_EXT)/Makefile
else
  include proto-standard/Makefile
endif

# WRMODE - Update 2012-11-24
# obsolete compilation parameter. No used when ppsi is compiled inside wrpc-sw.
# Still used only by ppsi standalone main. In this case, set WRMODE=master|slave
WRMODE = none
ifeq ($(WRMODE), master)
CFLAGS += -DPPSI_MASTER
else ifeq ($(WRMODE), slave)
CFLAGS += -DPPSI_SLAVE
endif

# VERB_LOG_MSGS: uncomment if you want very verbose msg when log verbosity=2.
# As default, they are not even compiled, in order to save space in the final
# binary executable
# VERB_LOG_MSGS=y

# Include arch code, the default is hosted GNU/Linux stuff
# we need this -I so <arch/arch.h> can be found
ARCH ?= gnu-linux
CFLAGS += -Iarch-$(ARCH)/include
include arch-$(ARCH)/Makefile

# And this is the rule to build our target.o file. The architecture may
# build more stuff. Please note that ./MAKEALL looks for $(TARGET)
# (i.e., the ELF which is either the  output or the input to objcopy -O binary)
#
# The object only depends on OBJ-y because each subdirs added needed
# libraries: see proto-standard/Makefile as an example.

$(TARGET).o: $(OBJ-y)
	$(LD) -Map $(TARGET).map1 -r -o $@ $(OBJ-y) $(LIBS)

# Finally, "make clean" is expected to work
clean:
	rm -f $$(find . -name '*.[oa]') *.bin $(TARGET) *~ $(TARGET).map*
