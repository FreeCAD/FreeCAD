# This is a GNU Meakefile - if 'make' fails (which it will on pure BSD
# systems), kick it off with 'gmake' instead.

COMMON_CPPFLAGS = -I.
# COMMON_CPPFLAGS = -I. -fPIC
# Add -fPIC to the compiler flags if necessary (misc 64-bit Linux platforms)

SWITCHBOARD_CPPFLAGS = -DSWITCHBOARD_INTERNAL $(COMMON_CPPFLAGS)
SWITCHBOARDOPERATOR_CPPFLAGS = -DSWITCHBOARDOPERATOR_INTERNAL $(COMMON_CPPFLAGS)

ifeq ($(shell uname | cut -c1-6), CYGWIN)
all: Switchboard.dll SwitchboardOperator.dll
else
all: Switchboard.so SwitchboardOperator.so
endif

clean:
	rm -f *.obj *.o *.pdb *.ilk *.lib *.dll *.exp *.so

Switchboard.dll: Switchboard.h Switchboard.cpp Makefile
	CPPFLAGS="$(SWITCHBOARD_CPPFLAGS)" coin-config --compile Switchboard.cpp
	wrapmsvc `coin-config --ldflags` -DLL /INCREMENTAL:NO /OUT:$@ Switchboard.obj `coin-config --libs`

SwitchboardOperator.dll: SwitchboardOperator.h SwitchboardOperator.cpp Switchboard.h Makefile
	CPPFLAGS="$(SWITCHBOARDOPERATOR_CPPFLAGS)" coin-config --compile SwitchboardOperator.cpp
	wrapmsvc -DLL /OUT:$@ SwitchboardOperator.obj -L. -lSwitchboard `coin-config --ldflags --libs`

Switchboard.so: Switchboard.h Switchboard.cpp Makefile
	CPPFLAGS="$(SWITCHBOARD_CPPFLAGS)" LDFLAGS="-shared" \
	coin-config --build $@ Switchboard.cpp

SwitchboardOperator.so: SwitchboardOperator.h SwitchboardOperator.cpp Switchboard.h Switchboard.so Makefile
	CPPFLAGS="$(SWITCHBOARDOPERATOR_CPPFLAGS)" LDFLAGS="-shared" \
	LIBS="Switchboard.so" \
	coin-config --build $@ SwitchboardOperator.cpp

