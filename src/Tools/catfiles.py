#! python
# -*- coding: utf-8 -*-
# (c) 2018 Werner Mayer LGPL
#

import sys, getopt

# import os # The code that needs this is commented out
import shutil


def main():
    outputfile = ""
    try:
        opts, args = getopt.getopt(sys.argv[1:], "o:", ["outputfile="])
    except getopt.GetoptError:
        pass

    for o, a in opts:
        if o in ("-o", "--outputfile"):
            outputfile = a

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

    with open(outputfile, "wb") as wfd:
        for f in args:
            with open(f, "rb") as fd:
                shutil.copyfileobj(fd, wfd, 1024 * 1024 * 10)
        print("Created file {0}".format(outputfile))


if __name__ == "__main__":
    main()
