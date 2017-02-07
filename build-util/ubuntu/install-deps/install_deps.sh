#!/bin/bash
# -*- coding: utf-8, tab-width: 2 -*-


function install_deps () {
  export LANG{,UAGE}=en_US.UTF-8  # make error messages search engine-friendly
  local SELFPATH="$(readlink -m "$BASH_SOURCE"/..)"
  cd "$SELFPATH" || return $?

  local HOWTO_WIKI='https://www.freecadweb.org/wiki/?title=CompileOnUnix'
  local HOWTO_HTML="tmp.$(date +%F).howto.html"
  echo -n "Downloading how-to from wiki: $HOWTO_HTML"
  if [ -s "$HOWTO_HTML" ]; then
    echo ' (skip: you already have it.)'
  else
    echo
    wget -c "$HOWTO_WIKI" -O "$HOWTO_HTML".part || return $?
    mv --verbose -- "$HOWTO_HTML"{.part,} || return $?
  fi

  echo 'Extract list of dependencies:'
  find_packages_in_howto || return $?

  return 0
}


function better_how () {
  local MODE="$1"; shift

  local SRCFN='--'
  case "$MODE" in
    /dev/fd/* )
      SRCFN="$MODE"
      MODE=;;
  esac
  BETTER="$(timeout --foreground 2s cat "$SRCFN")"

  case "$MODE" in
    '' ) ;;
    sed )
      [ -n "$HOW" ] || echo "W: $FUNCNAME $MODE: empty input" >&2
      [ -n "$BETTER" ] || echo "W: $FUNCNAME $MODE: empty script" >&2
      BETTER="$( LANG=C "$MODE" "$@" -rf <(echo "$BETTER"
        ) -- <(echo "$HOW") )";;
    * )
      echo "E: $FUNCNAME: unsupported mode: $MODE" >&2
      BETTER=;;
  esac

  if [ -n "$BETTER" ]; then HOW="$BETTER"; return 0; fi
  echo "D: $(wc -l <<<"$HOW") line(s) of how-to were lost in $(
    readlink -m "${BASH_SOURCE[1]}") line ${BASH_LINENO[0]}."
  return 2
}


function find_packages_in_howto () {
  local MAXLN=9002
  local HEADLINE_RX='<h3>?[^>\n]* class="mw-headline" id="'
  local HOW=
  better_how <(grep -Pe "${HEADLINE_RX}Debian_and_Ubuntu" -m 1 -A $MAXLN \
    -- "$HOWTO_HTML" | grep -Pe "$HEADLINE_RX" -m 2 -B $MAXLN
    ) || return $?

  local SED_READ_ALL=$': read_all\n$!{N; b read_all}'
  local UBU_VER_RX='[0-9]+\.[0-9]+'
  local UBU_VER_RANGE_RX='\bUbuntu ('"$UBU_VER_RX|$UBU_VER_RX"' and [a-z]+)\b'
  local SED_CMD="$SED_READ_ALL"'
    s~<ul>~\a~;s~^[^\a]*\a~~

    # merge adjacent package name lines
    s~\s*(</li>)\s*(<li>)~\1\2~g

    # Prepare to decide about the indented lists:
    s~\s*</p>\s*<dl>\s*<dd>~~g
    s~\s*(</(dl|p)>)~\1\f\n~g
    s~\s*(<p>)~\n\1~g

    s~<p>Optional[^<>]* (extra packages:)</p>\s*<ul>~<!-- \1 -->\n~

    s~\s*<p>Note that (\S+) includes [^\f]+\f(\s|<ul>|<li>[^<>]*</li>|$\
      )+</ul>\s*<p>You may have to [^<>]*</p>~\n<!-- \1: noted. -->~g

    s~ \(([^\f<>]*, |)'"$UBU_VER_RANGE_RX"'(,[^\f<>]*|)\)(</li>|$\
      )\s*</ul>\s*</dd>~<ubuntu=\2>\4~g

    s~ \(opencascade community [^()\f]*\)~<want=no>~
      # ^-- b/c we cannot easily use asterisk in package names
    s~ \(official opencascade [^()\f]*\)~<want=yes>~
    s~ \((to|if you|for|needed for) [^<>\f]*\)</li>~</li>~g

    # Simplify HTML
    s~(<a)\b[^<>]*( href="[^<>"]+")[^<>]*>~\1\2>~g
    s~\&amp;~\&~g
    '
  better_how sed <<<"$SED_CMD" || return $?

  # Help with some of the additional instructions.
  SED_CMD='/^<p>/{
    s~\a~~g
    s~<a\b[^<>]* href="([^<>"]+)"[^<>]*>(Additional instructions?|$\
      )[\:\. ]*</a>[\:\. ]*(.*)$~\2: \3\a\1~i
    s~(\s|</p>)*\a~ \a~
    s~'"$UBU_VER_RANGE_RX"'~<ubuntu=\1>&~
    s~\a~~g
    }'
  better_how sed <<<"$SED_CMD" || return $?

  # Decipher Ubuntu version ranges
  SED_CMD='
    s~… and (before|earlier)>~\1…\2\3>~g
    s~… and (forward|later)>~\1\2\3…>~g
    s~…>~\1\2\3>~g
    '
  SED_CMD="${SED_CMD//  s~…/s~(<ubuntu=)([0-9]+)\\.([0-9]+)}"
  better_how sed <<<"$SED_CMD" || return $?
  # echo "$HOW"; return $?

  # resolve Ubuntu version decisions
  [ -n "$UBU_VER_SERIAL" ] || local UBU_VER_SERIAL="$(
    lsb_release -sr | grep -oPe '^\d*\.\d+' | sed -re 's~\.~~;s~^0+~~')"
  SED_CMD='
    s~<ubuntu=…1304>~<want=yes>~g
    s~<ubuntu=1310…>~<want=no>~g
    '
  [ "$UBU_VER_SERIAL" -gt 1310 ] && SED_CMD='
    s~<ubuntu=…1304>~<want=no>~g
    s~<ubuntu=1310…>~<want=yes>~g
    '
  SED_CMD+='
    s~<ubuntu='"$UBU_VER_SERIAL"'>~<want=yes>~g
    s~<ubuntu=[0-9]{3,4}>~<want=no>~g
    /<want=no>/d
    s~^<p>[^<>\f]*<ul>(.*)<want=yes>~\1~
    '
  better_how sed <<<"$SED_CMD" || return $?


  SED_CMD="$SED_READ_ALL"'
    s~\f\s*</dl>\s*(<ul>\s*|)~\n~g
    s~\f~\n&!!\n~g
    s~(^|\n)(<ul>)+~\1~g
    s~(</ul>|</dd>|</dl>)+(\n|$)~\2~g
    s~(\s|</?div\b[^<>]*>)+<h3>(</?span\b[^<>]*>|[^<>]+)*</h3>\s*$~~

    # identify what could be a package
    s~<li><%pkgn>\s*</li>~<pkg>\1\n~g
    s~<li><%pkgn>\s+and\s+<%pkgn>\s*</li>~<pkg>\1\n<pkg>\2\n~g
    '
  SED_CMD="${SED_CMD//<%pkgn>/([a-z][a-z0-9+-]+)}"
  better_how sed <<<"$SED_CMD" || return $?

  # last exit for debug:
  [ "${DEBUGLEVEL:-0}" -ge 2 ] && echo "$HOW" && return $?
  # from here output is heavily transformed and might not help you
  # determine what to match for.

  SED_CMD="$SED_READ_ALL"'
    s~\s*\f+!+(\n|$)~\1~g
    s~\n!+\n~\n~g
    s~\s*\n\s*~\n~g
    '
  better_how sed <<<"$SED_CMD" || return $?

  local PKG=()
  readarray -t PKG < <(<<<"$HOW" sed -nre 's~^<pkg>~~p
    ' | LANG=C sort | tee dep-pkgs.lst)
  local MSG=()
  readarray -t MSG < <(<<<"$HOW" grep -vPe '^<(pkg>|!--)' | tee dep-pkgs.msg)

  echo -n "Found ${#PKG[@]} package names and ${#MSG[@]} other messages"
  if [ "${#MSG[@]}" == 0 ]; then
    echo '.'
    return 0
  fi

  echo ':'
  printf '%s\n' "${MSG[@]}" | nl -ba | sed -re '1!s~^~\n~
    ' | LANG=C sed -re '
    s~</?p>~~g
    s~<a href="([^<>"]+)">([^<>]*)</a>~\2 (\1)~g
    ' | fmt -t | sed -re '/^$/d;s~^\S~\t&~'
  echo "You'll have to resolve these issues manually. :-(" \
    'When you did, you can run: sudo apt-get install $(cat dep-pkgs.lst)'
  return 4
}










[ "$1" == --lib ] && return 0; install_deps "$@"; exit $?
