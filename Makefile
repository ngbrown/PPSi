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

# Instead of repeating "proto-daemon" over and over, bless it TARGET
TARGET = ppsi

# The main target is the big object file.
all: $(TARGET).o

# CFLAGS to use. Both this Makefile (later) and app-makefile may grow CFLAGS
CFLAGS += -Wall -O2 -ggdb -Iinclude

# to avoid ifdef as much as possible, I use the kernel trick for OBJ variables
OBJ-y := fsm.o

# include diagnostic objects
include diag/Makefile

# proto-standard is always included
include proto-standard/Makefile

# including the extension Makefile, if an extension is there
ifdef PROTO_EXT
  include proto-ext-$(PROTO_EXT)/Makefile
endif

WRMODE = slave
ifeq ($(WRMODE), master)
CFLAGS += -DPPSI_MASTER
else
CFLAGS += -DPPSI_SLAVE
endif

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
