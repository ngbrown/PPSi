# Hosted environment: build the final exectuable

# All files are under D: I'm lazy
W := proto-ext-whiterabbit

LIBWR := $W/libwr.a

LIBS += -L$W -lwr

OBJ-libwr := $W/fsm-table.o \
	$W/state-initializing.o \
	$W/state-faulty.o \
	$W/state-disabled.o \
	$W/state-listening.o \
	$W/state-pre-master.o \
	$W/state-master.o \
	$W/state-passive.o \
	$W/state-uncalibrated.o \
	$W/state-slave.o \
	$W/common-fun.o \
	$W/bmc.o \
	$W/msg.o \
	$W/arith.o \
	$W/servo.o \
	$W/open-close.o \
	$W/state-wr-present.o \
	$W/state-wr-m-lock.o \
	$W/state-wr-s-lock.o \
	$W/state-wr-locked.o \
	$W/state-wr-calibration.o \
	$W/state-wr-calibrated.o \
	$W/state-wr-resp-calib-req.o \
	$W/state-wr-link-on.o \
	$W/wr_msg.o

$(TARGET).o: $(LIBWR)

$(LIBWR): $(OBJ-libwr)
	$(AR) r $@ $^
