#!/bin/bash
## Create resources stuff for Plot module.

# Perform trnaslations
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot.ts
lrelease resources/translations/Plot.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_af.ts
lrelease resources/translations/Plot_af.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_cs.ts
lrelease resources/translations/Plot_cs.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_de.ts
lrelease resources/translations/Plot_de.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_es-ES.ts
lrelease resources/translations/Plot_es-ES.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_fi.ts
lrelease resources/translations/Plot_fi.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_fr.ts
lrelease resources/translations/Plot_fr.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_hr.ts
lrelease resources/translations/Plot_hr.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_hu.ts
lrelease resources/translations/Plot_hu.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_it.ts
lrelease resources/translations/Plot_it.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_ja.ts
lrelease resources/translations/Plot_ja.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_nl.ts
lrelease resources/translations/Plot_nl.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_no.ts
lrelease resources/translations/Plot_no.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_pl.ts
lrelease resources/translations/Plot_pl.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_pt-BR.ts
lrelease resources/translations/Plot_pt-BR.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_ro.ts
lrelease resources/translations/Plot_ro.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_ru.ts
lrelease resources/translations/Plot_ru.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_sk.ts
lrelease resources/translations/Plot_sk.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_sv-SE.ts
lrelease resources/translations/Plot_sv-SE.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_tr.ts
lrelease resources/translations/Plot_tr.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_uk.ts
lrelease resources/translations/Plot_uk.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_zh-CN.ts
lrelease resources/translations/Plot_zh-CN.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Plot_zh-TW.ts
lrelease resources/translations/Plot_zh-TW.ts

# Create resources
rm -f Plot_rc.py
cd resources
pyrcc4 Plot.qrc -o ../Plot_rc.py
cd ..
