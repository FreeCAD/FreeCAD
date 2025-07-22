#! python
# (c) 2009 Juergen Riegel GPL

Usage = """examplePy2wiki - generating a wiki text out of a python example

Usage:
   examplePy2wiki [Optionen]

Options:
 -o  --out-file=FILENAME  use this file name for output, default resources.qrc
 -i, --in-file=FILENAME   directory to search, default PWD
 -h, --help               print this help message

This program reads python files and generate a output suited for a Mediawiki page.
The python comments get translated to text and the code blocks get intended to
show up us code in the wiki.


Author:
  (c) 2009 Juergen Riegel
  juergen.riegel@web.de
  Licence: GPL V2

Version:
  0.1
"""

import os, sys, string, getopt


def Process(line):
    if line[0:2] == "# ":
        return line[2:]
    else:
        return " " + line


def main():

    try:
        opts, args = getopt.getopt(
            sys.argv[1:], "hi:o:", ["help", "verbose", "in-file=", "out-file="]
        )
    except getopt.GetoptError:
        # print help information and exit:
        sys.stderr.write(Usage)
        sys.exit(2)

    # checking on the options
    for o, a in opts:
        if o in ("-h", "--help"):
            sys.stderr.write(Usage)
            sys.exit()
        if o in ("-o", "--out-file"):
            outfile = open(a, "w")
        if o in ("-i", "--in-file"):
            infile = open(a, "r")

    lines = infile.readlines()
    for l in lines:
        outfile.write(Process(l))
        # print l


if __name__ == "__main__":
    main()
