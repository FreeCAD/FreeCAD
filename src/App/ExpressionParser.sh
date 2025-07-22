#!/usr/bin/env sh

(cd "$(dirname "$0")" && \
  flex -v -oExpression.lex.c Expression.l && \
  bison -d -v -Wall -oExpression.tab.c Expression.y && \
  sed -i '1s|^|// clang-format off\n|' Expression.tab.c && \
  sed -i '1s|^|// clang-format off\n|' Expression.lex.c \
)
