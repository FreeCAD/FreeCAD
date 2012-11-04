#!/bin/bash
## Create resources stuff for Plot module.

# Perform trnaslations
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot.ts
lrelease resources/translations/Plot.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_es-ES.ts
lrelease resources/translations/Plot_es-ES.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_es.ts
lrelease resources/translations/Plot_es.ts

# Create resources
rm -f Plot_rc.py
cd resources
pyrcc4 Plot.qrc -o ../Plot_rc.py
cd ..
