# Alessandro Rubini for CERN, 2011 -- public domain

CFLAGS += -ffreestanding -Os  \
	-ffunction-sections -fdata-sections \
	-mmultiply-enabled -mbarrel-shift-enabled

ARCH_LDFLAGS = -nostdlib -static -T $(ARCH).lds

# All files are under A (short for ARCH): I'm lazy
A := arch-$(ARCH)

LIBARCH := $A/libarch.a

OBJ-libarch := $A/spec-startup.o \
	$A/main-loop.o \
	$A/spec-socket.o \
	$A/spec-io.o \
	$A/spec-timer.o \
	$A/dev/uart.o \
	$A/dev/syscon.o \
	$A/dev/ep_pfilter.o \
	$A/dev/endpoint.o \
	$A/dev/pps_gen.o \
	$A/dev/minic.o \
	$A/dev/softpll_ng.o \
	$A/dev/irq.o \
	lib/div64.o

$(LIBARCH): $(OBJ-libarch)
	$(AR) r $@ $^

all: $(TARGET) $(TARGET).bin

# to build the target, we need -lstd again, in case we call functions that
# were not selected yet (e.g., pp_open_instance() ).
$(TARGET): $(TARGET).o $A/crt0.o $(LIBARCH)
	$(CC) -Wl,-Map,$(TARGET).map2 $(ARCH_LDFLAGS) -o $@ $A/crt0.o \
		$(TARGET).o -L$A -larch -L$D -lstd -L$W -lwr

$(TARGET).bin: $(TARGET)
	$(OBJCOPY) -O binary $^ $@
