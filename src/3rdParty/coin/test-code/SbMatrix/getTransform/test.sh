#!/bin/sh

if test getTransform -ot getTransform.cpp
then
  coin-config --build getTransform getTransform.cpp || exit 1
fi

./getTransform < input.txt > out.txt
diff -u out.txt result.txt
rm -f out.txt
exit 0
