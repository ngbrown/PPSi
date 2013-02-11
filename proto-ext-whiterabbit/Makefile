# Hosted environment: build the final exectuable

# All files are under directory D: I'm lazy
D := proto-ext-whiterabbit

LIBWR := $D/libwr.a

LIBS += -L$D -lwr

OBJ-libwr := $D/fsm-table.o \
	$D/state-initializing.o \
	$D/state-faulty.o \
	$D/state-disabled.o \
	$D/state-listening.o \
	$D/state-master.o \
	$D/state-uncalibrated.o \
	$D/state-slave.o \
	$D/common-fun.o \
	$D/bmc.o \
	$D/msg.o \
	$D/arith.o \
	$D/servo.o \
	$D/open-close.o \
	$D/state-wr-present.o \
	$D/state-wr-m-lock.o \
	$D/state-wr-s-lock.o \
	$D/state-wr-locked.o \
	$D/state-wr-calibration.o \
	$D/state-wr-calibrated.o \
	$D/state-wr-resp-calib-req.o \
	$D/state-wr-link-on.o \
	$D/wr-msg.o \
	$D/wr-servo.o

$(TARGET).o: $(LIBWR)

$(LIBWR): $(OBJ-libwr)
	$(AR) r $@ $^
