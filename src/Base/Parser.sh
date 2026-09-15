# SPDX-License-Identifier: LGPL-2.1-or-later

(cd "$(dirname "$0")" && \
  flex -oQuantity.lex.c Quantity.l && \
  bison -oQuantity.tab.c Quantity.y && \
  sed -i '1s|^|// SPDX-License-Identifier: LGPL-2.1-or-later AND GPL-3.0-or-later WITH Bison-exception-2.2\n\n// clang-format off\n|' Quantity.tab.c && \
  sed -i '1s|^|// SPDX-License-Identifier: LGPL-2.1-or-later\n\n// clang-format off\n|' Quantity.lex.c \
)
