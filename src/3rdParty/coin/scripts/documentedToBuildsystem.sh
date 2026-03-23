#!/bin/bash

#To use this script, either give no parameters for generating a normal
#documentation build, or give the modules you want to include as
#parameters.

#documentedToBuildsystem.sh [submodule1] [submodule2] [...]

error()
{
    echo $@ > /dev/stderr
}

cd $(dirname $0)/..

EXTRA_INCLUDES='${coin_build_dir}/include/Inventor/C/basic.h
${coin_src_dir}/docs/releases.dox
'

if [ -n "$1" ]
then
    #Should be included always
    SUBMODULES="src/doc"

    while [ -n "$1" ]
    do
        failed=true;
        if [ -d "src/$1" ]
        then
            SUBMODULES="${SUBMODULES} src/$1"
            failed=false;
        fi
        if [ -d "include/Inventor/$1" ]
        then
            SUBMODULES="${SUBMODULES} include/Inventor/$1"
            failed=false;
        fi
        if ${failed}
        then
            error "No such module: $1"
            exit 1
        fi
        shift
    done
else
    SUBMODULES="src include"
fi

(
echo 'DOXYGEN_INPUT="'

(
for file in ${EXTRA_INCLUDES}
do
    echo "                         \${path_tag}${file}"
done
for file in $(find ${SUBMODULES} docs -name '*.cpp' -or -name '*.h' -or -name '*.dox' | xargs fgrep -l '/*!' )
do
    echo "                         \${path_tag}\${coin_src_dir}/${file}"
    CLASS=$(echo $file | egrep '\.cpp$'| rev| cut -d/ -f1 | cut -d. -f2- | rev)
    if [ -n "${CLASS}" ]
    then
        HEADER=$(find include -name "${CLASS}.h" -or -name "SoVRML${CLASS}.h" )
        if [ -z "${HEADER}" ]
        then
            N=0
        else
            N=$(echo "${HEADER}" | wc -l)
        fi
        if [ ${N} -ne 1 ]
        then
            if [ ${N} -gt 1 ]
            then
                HEADER_NEW=""
                for header in ${HEADER}
                do
                    header2=$(echo ${header} | cut -d '/' -f2-)
                    if ! grep -q "#error Do not include ${header2}" ${header}
                    then
                        HEADER_NEW="${HEADER_REV}${header} "
                    else
                        let N--
                    fi
                done
                HEADER=$(echo ${HEADER_NEW} | rev | cut -d\  -f2- | rev)
            fi
        fi

        if [ ${N} -eq 1 ]
        then
            echo "                         \${path_tag}\${coin_src_dir}/${HEADER}"
        elif [ ${N} -gt 1 ]
        then
            error "Error: ${HEADER}"
        fi
    fi
done
) | sort | uniq
echo '"'
) > docs/coin_doxygenfiles.inc
