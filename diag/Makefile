# Alessandro Rubini for CERN, 2011 -- public domain
# This changed in 2013-02, and it's incompatible with what we had

OBJ-y += diag/diag-yes.o

ifdef HAS_DIAG
  SAY := $(shell echo "WARNING: ppsi doesn't use \$$HAS_DIAG any more" >&2)
endif
ifdef HAS_FULL_DIAG
  SAY := $(shell echo "WARNING: ppsi doesn't use \$$HAS_FULL_DIAG any more" >&2)
endif