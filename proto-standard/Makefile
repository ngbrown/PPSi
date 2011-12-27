# Hosted environment: build the final exectuable

# All files are under D: I'm lazy
D := proto-standard

LIBSTD := $D/libstd.a

LIBS += -L$D -lstd

OBJ-libstd := $D/state-table-default.o \
	$D/state-initializing.o \
	$D/state-faulty.o \
	$D/state-disabled.o \
	$D/state-listening.o \
	$D/state-pre-master.o \
	$D/state-master.o \
	$D/state-passive.o \
	$D/state-uncalibrated.o \
	$D/state-slave.o \
	$D/state-common-fun.o \
	$D/bmc.o \
	$D/msg.o \
	$D/open-close.o

$(TARGET).o: $(LIBSTD)

$(LIBSTD): $(OBJ-libstd)
	$(AR) r $@ $^
