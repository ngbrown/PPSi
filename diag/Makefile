# Alessandro Rubini for CERN, 2011 -- public domain
# Diagnostics: we have tree levels: none, full and limited

# set HAS_FULL_DIAG as default (mainly for debugging purpose)
HAS_FULL_DIAG ?= y

# full: use a complete printf and the diag code
OBJ-$(HAS_FULL_DIAG) += diag/diag-yes.o diag/diag-printf.o diag/printf-full.o

# limited: use a minimal printf (can't coexist with FULL_DIAG)
ifndef HAS_FULL_DIAG
  OBJ-$(HAS_DIAG) += diag/diag-yes.o  diag/diag-printf.o diag/printf-mini.o
endif

# no diagnostics: the diag-no.c file includes weak aliases to an empty func
OBJ-y += diag/diag-no.o
