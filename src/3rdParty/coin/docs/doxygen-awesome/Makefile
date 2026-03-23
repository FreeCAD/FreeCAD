# SPDX-FileCopyrightText: 2022 Andrea Pappacoda <andrea@pappacoda.it>
# SPDX-License-Identifier: MIT

.POSIX:

PROJECT = doxygen-awesome-css

# Paths
PREFIX = /usr/local
DATADIR = share
INSTALLDIR = $(DESTDIR)$(PREFIX)/$(DATADIR)/$(PROJECT)

# Utilities
INSTALL = install -m 644
MKDIR = mkdir -p
RM = rm -f

# Files to be installed
FILES = doxygen-awesome-darkmode-toggle.js \
  doxygen-awesome-fragment-copy-button.js \
  doxygen-awesome-interactive-toc.js \
  doxygen-awesome-paragraph-link.js \
  doxygen-awesome-sidebar-only-darkmode-toggle.css \
  doxygen-awesome-sidebar-only.css \
  doxygen-awesome-tabs.js \
  doxygen-awesome.css

# Empty targets so that `make` and `make clean` do not cause errors
all:
clean:

install:
	$(MKDIR) $(INSTALLDIR)
	$(INSTALL) $(FILES) $(INSTALLDIR)/

uninstall:
	$(RM) -r $(INSTALLDIR)/

.PHONY: all clean install uninstall
