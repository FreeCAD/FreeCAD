#!/usr/bin/env sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

(cd "$(dirname "$0")" && \
  flex -v -oExpression.lex.c Expression.l && \
  bison -d -v -Wall -oExpression.tab.c -Wcounterexamples -Wconflicts-sr Expression.y && \
  sed -i '1s|^|// clang-format off\n|' Expression.tab.c && \
  sed -i '1s|^|// clang-format off\n|' Expression.lex.c \
)
