cd $(dirname $0)/../..
export COINDIR=$(cygpath -w $(pwd))
build/general/generate-all.sh Coin4 7 9
