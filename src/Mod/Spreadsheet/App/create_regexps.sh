#!/bin/sh

# DerivedGeneralCategory.txt used below can be downloaded from
# http://www.unicode.org/Public/UNIDATA/extracted/DerivedGeneralCategory.txt

exec 2>/dev/null

export PATH=../../../../build/src/Mod/Spreadsheet/App:$PATH

for l in Cc Cf Cn Co Cs Ll Lm Lo Lt Lu Mc Me Mn Nd Nl No Pc Pd Pe Pf Pi Po Ps Sc Sk Sm So Zl Zp Zs ; do
    grep "; $l" DerivedGeneralCategory.txt > $l.txt
    genregexps $l $l.txt
done

genregexps L Ll.txt Lm.txt Lo.txt Lt.txt Lu.txt
genregexps C Cc.txt Cf.txt Cn.txt Co.txt Cs.txt
genregexps M Mc.txt Me.txt Mn.txt
genregexps N Nd.txt Nl.txt No.txt
genregexps P Pc.txt Pd.txt Pe.txt Pf.txt Pi.txt Po.txt Ps.txt
genregexps S Sc.txt Sk.txt Sm.txt So.txt
genregexps Z Zl.txt Zp.txt Zs.txt
