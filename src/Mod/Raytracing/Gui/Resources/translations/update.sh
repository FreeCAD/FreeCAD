#!/bin/bash
#
# Tested in Ubuntu 20.04.2 LTS.
# This script must strictly contain line endings in Linux format for correct work.
# Install QT5 dev tools packages before run this script, by the following commands:
#
# sudo apt update
# sudo apt install -y qttools5-dev-tools
# sudo apt install -y pyqt5-dev-tools
#
# This is array of supported languages. New languages, must be added to it.
languages=(zh-TW)
for lang in ${languages[*]}
do
   # Check if fastners_$lang.ts exist
   if [ -f "Raytracing_$lang.ts" ]; then
      echo -e '\033[1;32m\n     <<< Update translation for '$lang' language >>> \n\033[m';
      # Creation of uifiles.ts file from ../*.ui files with designation of language code
#      lupdate6 ../preferences/*.ui -ts uifiles.ts -source-language en_US -target-language $lang -no-obsolete
      lupdate6 ../../*.ui  -ts uifiles.ts -source-language en_US -target-language $lang -no-obsolete
      pylupdate6  ../../../*.py -ts pyfiles.ts --verbose
      # Join uifiles.ts and pyfiles.ts files to Raytracing.ts
      lconvert -i uifiles.ts pyfiles.ts -o Raytracing.ts --verbose
      # Join Raytracing.ts to exist Raytracing_(language).ts file ( -no-obsolete)
      lconvert -i RaytracingOrg.ts Raytracing.ts Raytracing_zh-TW.po -o Raytracing_$lang.ts -target-language zh_TW -sort-contexts --verbose
#      lconvert -i Raytracing.ts Raytracing_$lang.ts -o Raytracing_$lang.po -of po -target-language $lang -sort-contexts
      lconvert -i Raytracing.ts -o Raytracing.pot -of pot -target-language en-US
      lconvert -i title.ts Raytracing_zh-TW.ts -o Raytracing_zh-TW.po -of po -target-language zh_TW --verbose
      # (Release) Creation of *.qm file from Raytracing_(language).ts
      lrelease Raytracing_$lang.ts
      # Delete unused files
#      rm uifiles.ts
      cd ..
      dir2qrc Raytracing
#      cd translations
      rm pyfiles.ts uifiles.ts
      # rm Raytracing.ts
#      lconvert -i Raytracing_$lang.po -if po -o Raytracing.pot -of pot -source-language en_US -sort-contexts
   else
      echo -e '\033[1;33m\n     <<< Create files for added '$lang' language >>> \n\033[m';
      # Creation of uifiles.ts file from ../*.ui files with designation of language code
      lupdate ../*.ui -ts uifiles.ts -source-language en_US -target-language $lang -no-obsolete
      # Creation of pyfiles.ts file from ../*.py files
      pylupdate5 ../*.py -ts pyfiles.ts -verbose -source-language en_US -target-language $lang -no-obsolete
      # Join uifiles.ts and pyfiles.ts files to Raytracing_$lang.ts
      lconvert -i uifiles.ts pyfiles.ts -o Raytracing_$lang.ts
      # Delete unused files
#      rm uifiles.ts
      rm pyfiles.ts
   fi
done
