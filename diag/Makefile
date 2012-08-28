# Alessandro Rubini for CERN, 2011 -- public domain
# Diagnostics: we have tree levels: none, full and limited

# full: use a complete printf and the diag code
OBJ-$(HAS_FULL_DIAG) += diag/diag-yes.o diag/diag-printf.o diag/printf-full.o

# limited: use a minimal printf (can't coexist with FULL_DIAG)
ifndef HAS_FULL_DIAG
  OBJ-$(HAS_DIAG) += diag/diag-yes.o  diag/diag-printf.o diag/printf-mini.o
endif

# no diagnostics: the diag-no.c file includes weak aliases to an empty func
OBJ-y += diag/diag-no.o
