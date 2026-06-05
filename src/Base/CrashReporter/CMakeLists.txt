# SPDX-License-Identifier: LGPL-2.1-or-later


set(CrashReporter_SRCS
    Format.h
    Reader.cpp
    Reader.h
)

add_library(CrashReporter SHARED ${CrashReporter_SRCS})
