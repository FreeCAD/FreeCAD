#!/usr/bin/env sh
# SPDX-License-Identifier: LGPL-2.1-or-later

(cd "$(dirname "$0")" && \
  flex -v -oExpression.lex.c Expression.l && \
  bison -d -v -Wall -oExpression.tab.c -Wcounterexamples -Wconflicts-sr Expression.y && \
  sed -i '1s|^|// SPDX-License-Identifier: LGPL-2.1-or-later AND GPL-3.0-or-later WITH Bison-exception-2.2\n\n// clang-format off\n|' Expression.tab.c && \
  sed -i '1s|^|// SPDX-License-Identifier: LGPL-2.1-or-later AND GPL-3.0-or-later WITH Bison-exception-2.2\n\n// clang-format off\n|' Expression.tab.h && \
  sed -i '1s|^|// SPDX-License-Identifier: LGPL-2.1-or-later\n\n// clang-format off\n|' Expression.lex.c \
)
