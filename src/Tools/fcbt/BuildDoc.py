# shell and operating system
import os, sys, FileTools

# sys.path.append( "..\Tools" )

# import FileTools

# line separator
ls = os.linesep
# path separator
ps = os.pathsep
# dir separator
ds = os.sep

# ====================================================================
# script assumes to run in src/Doc
# os.chdir("e:/Develop/FreeCADWin/src/Doc")
LogFile = open("MakeDoc.log", "w")
if not os.path.isdir("../../doc"):
    os.mkdir("../../doc")
# if not os.path.isdir("../../Doc/res"):
#    os.mkdir("../../Doc/res")
# FileTools.cpfile("index.html","../../doc/index.html")
# FileTools.cpfile("FreeCAD.css","../../doc/res/FreeCAD.css")

# ====================================================================
sys.stdout.write("Running source documentation ...")
# running doxygen with the parameters from the config file
param = "doxygen fcbt" + ds + "BuildDocDoxy.cfg"
LogFile.write(param)
print(param)
text = os.popen(param).read()
LogFile.write(text)
if not os.path.isdir("../../doc/SourceDocumentation"):
    os.mkdir("../../doc/SourceDocumentation")

# ====================================================================
sys.stdout.write(" done\n  Generate HTML ...")
FileTools.cpall("html", "../../doc/SourceDocumentation")

"""
#====================================================================
sys.stdout.write(' done\n  Generate DVI ...')
os.chdir("latex")
text = os.popen("latex refman.tex").read()
LogFile.write(text)
text = os.popen("makeindex refman.idx").read()
LogFile.write(text)
text = os.popen("latex refman.tex").read()
text = os.popen("latex refman.tex").read()
text = os.popen("latex refman.tex").read()
FileTools.cpfile("refman.dvi","../../../doc/FrameWork/FrameWork.dvi")

#====================================================================
sys.stdout.write (' done\n  Generate PS ...')
text = os.popen("dvips refman.dvi").read()
LogFile.write(text)
FileTools.cpfile("refman.ps","../../../doc/FrameWork/FrameWork.ps")

#====================================================================
sys.stdout.write (' done\n  Generate PDF ...')
text = os.popen("pdflatex refman.tex").read()
LogFile.write(text)
FileTools.cpfile("refman.pdf","../../../doc/FrameWork/FrameWork.pdf")
os.chdir("..")

#====================================================================
sys.stdout.write (' done\n  Clean up temporary files ...')
FileTools.rmall("html")
FileTools.rmall("latex")

#====================================================================
sys.stdout.write (' done\nCreating manuals\n')
if not os.path.isdir("../../Doc/Manuals"):
    os.mkdir("../../Doc/Manuals")
os.chdir("Manuals")

#====================================================================
sys.stdout.write('  Generate DVI ...')
text = os.popen("latex Design_Specification.tex").read()
LogFile.write(text)
text = os.popen("makeindex Design_Specification.idx").read()
LogFile.write(text)
text = os.popen("latex Design_Specification.tex").read()
text = os.popen("latex Design_Specification.tex").read()
text = os.popen("latex Design_Specification.tex").read()
FileTools.cpfile("Design_Specification.dvi","../../../doc/Manuals/Design_Specification.dvi")

text = os.popen("latex Manual.tex").read()
LogFile.write(text)
text = os.popen("makeindex Manual.idx").read()
LogFile.write(text)
text = os.popen("latex Manual.tex").read()
text = os.popen("latex Manual.tex").read()
text = os.popen("latex Manual.tex").read()
FileTools.cpfile("Manual.dvi","../../../doc/Manuals/Manual.dvi")

#====================================================================
sys.stdout.write (' done\n  Generate PS ...')
text = os.popen("dvips Design_Specification.dvi").read()
LogFile.write(text)
FileTools.cpfile("Design_Specification.ps","../../../doc/Manuals/Design_Specification.ps")
text = os.popen("dvips Manual.dvi").read()
LogFile.write(text)
FileTools.cpfile("Manual.ps","../../../doc/Manuals/Manual.ps")

#====================================================================
sys.stdout.write (' done\n  Generate PDF ...')
text = os.popen("pdflatex Design_Specification.tex").read()
LogFile.write(text)
FileTools.cpfile("Design_Specification.pdf","../../../doc/Manuals/Design_Specification.pdf")
text = os.popen("pdflatex Manual.tex").read()
LogFile.write(text)
FileTools.cpfile("Manual.pdf","../../../doc/Manuals/Manual.pdf")

#====================================================================
#== run latex2html now NOTE: current directory MUST NOT contain any spaces !!!
sys.stdout.write (' done\n  Generate HTML ...')
if not os.path.isdir("../../../doc/Manuals/Design_Specification"):
    os.mkdir("../../../doc/Manuals/Design_Specification")
text = os.popen("latex2html Design_Specification.tex").read()
LogFile.write(text)
# if latex2html failed this directory doesn't exist
if os.path.isdir("Design_Specification"):
    FileTools.cpall("Design_Specification","../../../doc/Manuals/Design_Specification")
else:
    sys.stderr.write("latex2html failed!\n")

if not os.path.isdir("../../../doc/Manuals/Manual"):
    os.mkdir("../../../doc/Manuals/Manual")
text = os.popen("latex2html Manual.tex").read()
LogFile.write(text)
# if latex2html failed this directory doesn't exist
if os.path.isdir("Manual"):
    FileTools.cpall("Manual","../../../doc/Manuals/Manual")

#====================================================================
os.chdir("..")
sys.stdout.write (' done\n  copy online help ...')
if not os.path.isdir("../../Doc/Online"):
    os.mkdir("../../Doc/Online")
FileTools.cpall("Online","../../Doc/Online")

#====================================================================
sys.stdout.write (' done\n  Clean up temporary files ...')
LogFile.close()
"""
# ====================================================================
FileTools.rmall("html")

# ====================================================================
sys.stdout.write(" done\nDocumentation done!\n")


# print text
