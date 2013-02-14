# Alessandro Rubini for CERN, 2011 -- public domain

CFLAGS += -ffreestanding -Os \
	-ffunction-sections -fdata-sections \
	-mmultiply-enabled -mbarrel-shift-enabled \

ARCH_LDFLAGS = -nostdlib -static -T $(ARCH).lds

# Root of wrpc-sw project
WRPCSW_ROOT ?= ..

CFLAGS += -I$(WRPCSW_ROOT)/include -I$(WRPCSW_ROOT)/softpll \
	  -I$(WRPCSW_ROOT)/boards/spec \

# FIXME ptp-noposix root; used temporarily until we clean up
PTPNOPOSIX_ROOT = $(WRPCSW_ROOT)/ptp-noposix

CFLAGS += -I$(PTPNOPOSIX_ROOT)/PTPWRd/dep -I$(PTPNOPOSIX_ROOT)/PTPWRd



# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

LIBARCH := $A/libarch.a

LIBS += -L$A -larch


OBJ-libarch := $A/spec-socket.o \
	$A/spec-io.o \
	$A/spec-timer.o \
	$A/spec-spll.o \
	$A/spec-calibration.o \
	lib/div64.o

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

$(TARGET).o: $(LIBARCH)

ppsi-bin: $(TARGET) $(TARGET).bin

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_instance() ).
$(TARGET): $(TARGET).o $A/crt0.o $(LIBARCH) $A/main-loop.o $A/spec-startup.o
	$(CC) -Wl,-Map,$(TARGET).map2 $(ARCH_LDFLAGS) -o $@ $A/crt0.o \
		$(TARGET).o $(specdev-objects) $A/main-loop.o $A/spec-startup.o \
		-L$A -larch $(LIBS)

$(TARGET).bin: $(TARGET)
	$(OBJCOPY) -O binary $^ $@
