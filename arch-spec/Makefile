# Alessandro Rubini for CERN, 2011 -- public domain

CFLAGS += -ffreestanding -Os  \
	-ffunction-sections -fdata-sections \
	-mmultiply-enabled -mbarrel-shift-enabled

ARCH_LDFLAGS = -nostdlib -static -T $(ARCH).lds

# Root of wrpc-sw project
WRPCSW_ROOT = ..

# FIXME ptp-noposix root; used till everything common would not be put in
# wrpc-sw
PTPNOPOSIX_ROOT = $(WRPCSW_ROOT)/ptp-noposix

# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

LIBARCH := $A/libarch.a

OBJ-libarch := $A/spec-startup.o \
	$A/main-loop.o \
	$A/spec-socket.o \
	$A/spec-io.o \
	$A/spec-timer.o \
	$A/spec-spll.o \
	$A/spec-calibration.o \
	$A/spec-halexp.o \
	lib/div64.o

OBJ-specdev := $(WRPCSW_ROOT)/dev/uart.o \
	$(WRPCSW_ROOT)/dev/syscon.o \
	$(WRPCSW_ROOT)/dev/ep_pfilter.o \
	$(WRPCSW_ROOT)/dev/endpoint.o \
	$(WRPCSW_ROOT)/dev/pps_gen.o \
	$(WRPCSW_ROOT)/dev/minic.o \
	$(WRPCSW_ROOT)/arch/lm32/irq.o \
	$(WRPCSW_ROOT)/dev/sdb.o \
	$(PTPNOPOSIX_ROOT)/softpll/softpll_ng.o \
	$(WRPCSW_ROOT)/lib/mprintf.o

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

all: $(TARGET) $(TARGET).bin

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_instance() ).
$(TARGET): $(TARGET).o $A/crt0.o $(LIBARCH)
	$(CC) -Wl,-Map,$(TARGET).map2 $(ARCH_LDFLAGS) -o $@ $A/crt0.o \
		$(TARGET).o $(OBJ-specdev) -L$A -larch -L$D -lstd -L$W -lwr

$(TARGET).bin: $(TARGET)
	$(OBJCOPY) -O binary $^ $@
