#!/bin/sh

if test setValue_SbMatrix -ot setValue_SbMatrix.cpp
then
  coin-config --build setValue_SbMatrix setValue_SbMatrix.cpp || exit 1
fi

./setValue_SbMatrix < input.txt > out.txt
diff -u out.txt result.txt
rm -f out.txt
exit 0
