
CFLAGS = -Wall -O2 -ggdb

OBJ = mtp_udp.o mtp_packet.o onestamp.o mtp_stamp.o
PRG = $(OBJ:.o=)
LIB = libmtp.a
LOBJ = stamp-funcs.o report.o

LDFLAGS = -L. -lmtp

all: $(OBJ) $(PRG)


$(LIB): $(LOBJ)
	ar r $@ $^

%: %.o $(LIB)
	$(CC) $*.o $(LDFLAGS) -o $@

clean:
	rm -f $(PRG) $(LIB) *.o *~