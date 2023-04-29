#! python
# -*- coding: utf-8 -*-
# (c) 2006 Werner Mayer LGPL
# FreeCAD report memory leaks script to get provide the log file of Visual Studio in more readable file.

import string, re

# Open the memory leak file
file = open("MemLog.txt")
lines = file.readlines()
file.close()

d = dict()
l = list()
for line in lines:
    r = re.search("\\(#\\s*\\d+\\)", line)
    if r is not None:
        s = line[r.start() : r.end()]
        t = re.search("^Leak", line)
        if t is not None:
            m = d[s]
            l.append(m)
        else:
            d[s] = line

file = open("MemLog_leaks.txt", "w")
for line in l:
    line = string.replace(line, "Alloc", "Leak")
    file.write(line)
file.close()
