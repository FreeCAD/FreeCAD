#!/usr/bin/python

import os,sys,string


if(len(sys.argv) < 2):
    sys.stdout.write("Convert translation files to source code.\n")
    sys.exit()

if(len(sys.argv) > 4):
    sys.stderr.write("Wrong Parameter\n  Usage:\n  tstocpp Infile.ts Outfile Var_name\n")
    sys.exit()

file = open(sys.argv[1])

if(len(sys.argv) > 2):
    out = open(sys.argv[2],"w");
else:
    out = sys.stdout

if(len(sys.argv) > 3):
	name = sys.argv[3]
else:
	name = "input"

lines = file.readlines()

out.write("\nstd::vector<const char*> " + name + ";\n")
out.write("\nconst std::vector<const char*>& Get" + name + "()\n{\n")

for line in lines:
    # remove new line
    line2 = string.rstrip(line)
    # replace special chars
    line2 = string.replace(line2,'\\','\\\\')
    line2 = string.replace(line2,'\"','\\\"')
    line2 = string.replace(line2,"\'","\\\'")

    
    # output 
    #out.write(line)
    out.write( '  ' + name + '.push_back(\"' + line2 + '\\n\");\n')

out.write("  return " + name + ";\n}\n")
