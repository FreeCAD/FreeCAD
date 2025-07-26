#! python
# -*- coding: utf-8 -*-
# (c) 2018 Werner Mayer LGPL
#
import os.path
import sys, getopt

# import os # The code that needs this is commented out
import shutil


def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "o:", ["outputfile="])
    except getopt.GetoptError:
        print("usage: catfiles.py -o <outputfile>")
        sys.exit(1)

    for o, a in opts:
        if o in ("-o", "--outputfile"):
            outputfile = a
            with open(outputfile, "wb") as wfd:
                for f in args:
                    with open(f, "rb") as fd:
                        shutil.copyfileobj(fd, wfd, 1024 * 1024 * 10)
                print(f"Created file {outputfile}")
    # if os.path.exists(outputfile):
    #    do_not_create = True
    #    ts = os.path.getmtime(outputfile)
    #    for f in args:
    #        if os.path.getmtime(f) > ts:
    #            do_not_create = False
    #            break
    #
    #    if do_not_create:
    #        print ("Up-to-date file {0}".format(outputfile))
    #        return


if __name__ == "__main__":
    main()
