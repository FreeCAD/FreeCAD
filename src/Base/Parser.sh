# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

(cd "$(dirname "$0")" && \
  flex -oQuantity.lex.c Quantity.l && \
  bison -oQuantity.tab.c Quantity.y && \
  sed -i '1s|^|// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.\n\n// clang-format off\n|' Quantity.tab.c && \
  sed -i '1s|^|// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.\n\n// clang-format off\n|' Quantity.lex.c \
)
