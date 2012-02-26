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

CFLAGS = -Wall -ggdb -I../include/pptp

PROGS = ptpdump

all: $(PROGS)

$(PROGS): $(wildcard *.h) $(wildcard ../include/pptp/*.h)

%: %.c
	$(CC) $(CFLAGS) $*.c $(LDFLAGS) -o $@

clean:
	rm -f $(PROGS) *.o *~

