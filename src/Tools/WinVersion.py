#! python
# -*- coding: utf-8 -*-
# (c) 2012 Juergen Riegel LGPL
#
# Script to create files used in Windows build
# uses SubWCRev.py for version detection#

import SubWCRev, getopt, sys, string


def main():

    input = ""
    output = "."

    try:
        opts, args = getopt.getopt(sys.argv[1:], "dso:", ["dir=", "src=", "out="])
    except getopt.GetoptError:
        pass

    for o, a in opts:
        if o in ("-d", "--dir"):
            print("The %s option is deprecated. Ignoring." % (o))
        if o in ("-s", "--src"):
            input = a
        if o in ("-o", "--out"):
            output = a
    git = SubWCRev.GitControl()

    if git.extractInfo(input, ""):
        print(git.hash)
        print(git.branch)
        print(git.rev[0:4])
        print(git.date)
        print(git.url)
        print(input)
        print(output)

        f = open(input, "r")
        o = open(output, "w")
        for line in f.readlines():
            line = string.replace(line, "$WCREV$", git.rev[0:4])
            line = string.replace(line, "$WCDATE$", git.date)
            line = string.replace(line, "$WCURL$", git.url)
            o.write(line)


if __name__ == "__main__":
    main()
