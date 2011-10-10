import os,sys,string
import FCFileTools

file  = sys.argv[1]
input = open(file)
input.close

lines = input.readlines()
output = open(file,"w")

for line in lines:
    if (string.find(line, '    if ( strcmp(KParts::DockMainWindow::className(), "KParts::DockMainWindow") != 0 )') != -1):
        line = string.replace(line, line, '    if ( strcmp(DockMainWindow::className(), "DockMainWindow") != 0 )\n')
    output.write(line)

output.close    
