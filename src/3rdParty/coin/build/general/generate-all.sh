#!/bin/sh

usage() {
  echo "Usage is:"
  echo "  $0 <SolutionName> [FirstMSVCRev] [LastMSVCRev]"
}

error() {
  echo $@ >/dev/stderr
}

die() {
  error $@
  exit 1
}

#Defaults
FirstMSVCRev=7
LastMSVCRev=10

if [ -z "$1" ]
then
  usage
  exit 1
fi

SolutionName=$1

[ "$2" -gt 0 ] 2>/dev/null && FirstMSVCRev=$2
[ "$3" -gt 0 ] 2>/dev/null && LastMSVCRev=$3

cd $(dirname $0)/../..
cd build/general
for i in $(seq ${FirstMSVCRev} ${LastMSVCRev})
do
  ./generate.bat msvc$i ${SolutionName} || die "Unable to generate solution for msvc$i"
done

#msvc6 is only our starting point, but cannot be built.
#Uncomment to keep the files.
#rm -rf msvc6
