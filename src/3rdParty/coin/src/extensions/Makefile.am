
if BUILD_WITH_SUPERGLU
SUPERGLUDIR = superglu
else
SUPERGLUDIR =
endif

SUBDIRS = $(SUPERGLUDIR)

DIST_SUBDIRS = . $(SUPERGLUDIR)

## by setting DIST_SUBDIRS like this, "make dist" will include or exclude
## superglu from the source distribution depending on whether it is included
## in the source repository we've configured from.  [20011029 larsa]

