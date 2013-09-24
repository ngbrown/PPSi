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

CFLAGS = -Wall -ggdb -I../include

PROGS = ptpdump adjtime jmptime chktime adjrate
LDFLAGS += -lrt

all: $(PROGS)

$(PROGS): $(wildcard *.h) $(wildcard ../include/ppsi/*.h)

%: %.c
	$(CC) $(CFLAGS) $*.c $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $*.c $(LDFLAGS) -o $@

ptpdump: dump-main.o dump-funcs.o
	$(CC) $(LDFLAGS) dump-main.o dump-funcs.o -o $@

clean:
	rm -f $(PROGS) *.o *~

