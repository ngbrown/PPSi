#!/bin/bash
# Warning! you neet to run with -d 000001 debug flags in order to use this
# strip utility with success. otherwise you will get some junk lines in the
# middle which will not be thrown away

if [ $# != 1 ]; then
  echo "Usage: $0 inputfile";
  exit 1;
fi

grep "Real" $1 | \
sed 's/diag-extension-[0-9]-SLAVE: Real ofm//g'
