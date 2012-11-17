#!/bin/bash
## Create resources stuff for Ship module.

# Perform trnaslations
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship.ts
lrelease resources/translations/Ship.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_af.ts
lrelease resources/translations/Ship_af.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_cs.ts
lrelease resources/translations/Ship_cs.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_de.ts
lrelease resources/translations/Ship_de.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_es-ES.ts
lrelease resources/translations/Ship_es-ES.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_fi.ts
lrelease resources/translations/Ship_fi.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_fr.ts
lrelease resources/translations/Ship_fr.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_hr.ts
lrelease resources/translations/Ship_hr.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_hu.ts
lrelease resources/translations/Ship_hu.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_it.ts
lrelease resources/translations/Ship_it.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_ja.ts
lrelease resources/translations/Ship_ja.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_nl.ts
lrelease resources/translations/Ship_nl.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_no.ts
lrelease resources/translations/Ship_no.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_pl.ts
lrelease resources/translations/Ship_pl.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_pt-BR.ts
lrelease resources/translations/Ship_pt-BR.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_ro.ts
lrelease resources/translations/Ship_ro.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_ru.ts
lrelease resources/translations/Ship_ru.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_sk.ts
lrelease resources/translations/Ship_sk.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_sv-SE.ts
lrelease resources/translations/Ship_sv-SE.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_tr.ts
lrelease resources/translations/Ship_tr.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_uk.ts
lrelease resources/translations/Ship_uk.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_zh-CN.ts
lrelease resources/translations/Ship_zh-CN.ts
pylupdate4 -verbose `find ./ -name "*.py"` -ts resources/translations/Ship_zh-TW.ts
lrelease resources/translations/Ship_zh-TW.ts

# Create resources
rm -f Ship_rc.py
cd resources
pyrcc4 Ship.qrc -o ../Ship_rc.py
cd ..
