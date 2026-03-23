#! /bin/bash
#
# This manages the generation of the Visual Studio build files for Windows.

if test x"$COINDIR" = x""; then
  echo "error: COINDIR must be set before running this script"
  exit
fi

if ! perl -e 'exit 0' 2>/dev/null ; then
  echo "You need to have perl installed to run this script"
  exit
fi

project_name=coin
project=${project_name}$(fgrep -i "${project_name}_MAJOR_VERSION=" ../../configure | cut -d= -f2)

function cleansolution() {
  name=$1;
  rm -f ${name}.dsw ${name}.sln;
}

function cleanproject() {
  name=$1;
  rm -f ${name}.dsp ${name}.vcproj;
}

proper=true;

cleansolution ${project}
cleanproject ${project}
cleanproject ${project}_install
cleanproject ${project}_uninstall
cleanproject ${project}_docs

rm -f install-headers.bat uninstall-headers.bat;

build_pwd=`pwd`;
build="`cygpath -w $build_pwd | sed -e 's/\\\\/\\\\\\\\/g'`";
build_pwd="`pwd | sed -e 's/\\//\\\\\\\\/g'`\\\\";

source_pwd=`cd ../..; pwd`;
source="`cygpath -w $source_pwd | sed -e 's/\\\\/\\\\\\\\/g'`";
source_pwd="`(cd ../..; pwd) | sed -e 's/\\//\\\\\\\\/g'`";

if $proper; then
  ../../configure --with-msvcrt=mtd --with-suffix=d \
    --enable-3ds-import \
    --enable-debug --enable-symbols \
    htmldir=../html || exit 1;
  mv src/config.h src/config-debug.h;

  ../../configure --enable-msvcdsp --with-msvcrt=mt \
    --enable-3ds-import \
    --disable-debug --disable-symbols --enable-optimization \
    || exit 1;
  mv src/config.h src/config-release.h;

  cp ../misc/config-wrapper.h src/config.h;
else
  ./config.status --file=cfg/gendsp.pl;
fi

make || exit 1

if $proper; then
  sed \
    -e "s/$build/./g" \
    -e "s/$build_pwd//g" \
    -e "s/$source/..\\\\../g" \
    -e "s/$source_pwd/..\\\\../g" \
    -e 's/$/\r/g' \
    <${project}.dsp >new.dsp

  mv new.dsp ${project}.dsp

  sed \
    -e "s/$build/./g" \
    -e "s/$build_pwd//g" \
    -e "s/$source/..\\\\../g" \
    -e "s/$source_pwd/..\\\\../g" \
    -e 's/$/\r/g' \
    <install-headers.bat >new.bat
  rm -f install-headers.bat
  mv -f new.bat ../misc/install-headers.bat

  sed \
    -e "s/$build/./g" \
    -e "s/$build_pwd//g" \
    -e "s/$source/..\\\\../g" \
    -e "s/$source_pwd/..\\\\../g" \
    -e 's/$/\r/g' \
    <uninstall-headers.bat >new.bat
  rm -f uninstall-headers.bat
  mv -f new.bat ../misc/uninstall-headers.bat
fi

make docs/coin.doxygen
mv docs/coin.doxygen docs/doxygen.bak
sed \
  -e "s/$build/./g" \
  -e "s/$build_pwd//g" \
  -e "s/$source/..\\\\../g" \
  -e "s/$source_pwd/..\\\\../g" \
  -e "s/GENERATE_HTML.*=.*NO/GENERATE_HTML = YES/g" \
  -e "s/HTML_OUTPUT.*=.*/HTML_OUTPUT = ..\\\\html/g" \
  -e "s/GENERATE_TAGFILE.*=.*/GENERATE_TAGFILE = ..\\\\html\\\\coin.tag/g" \
  -e 's/$/\r/g' \
  <docs/doxygen.bak >docs/${project}.doxygen
rm docs/doxygen.bak

echo "Done."
echo "Make sure the ${project}.dsp file does not contain any absolute paths."
echo "Here are some indicator tests."
echo ""

set -x
grep -i "\\(c:\\|@\\)" ${project}.dsp /dev/null
grep -i "cygdrive" ${project}.dsp /dev/null
grep "hg" ${project}.dsp /dev/null
sort ../misc/install-headers.bat | uniq -c | grep -v " 1 "
grep "SOURCE=" ${project}.dsp | sort | uniq -c | grep -v " 1 "
set +x

echo ""
echo "If everything seems OK, upgrade the project files (if necessary) with"
echo "  devenv.exe (for msvc7 and msvc8), or"
echo "  VCExpress.exe (for msvc9)."
echo ""
