#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************






# here the usage description if you use this tool from the command line ("__main__")
CommandlineUsage = """Material - Tool to work with FreeCAD Material definition cards

Usage:
   Material [Options] card-file-name
   
Options:
 -c, --output-csv=file-name     write a comma seperated grid with the material data
 
Exit:
 0      No Error or Warning found
 1      Argument error, wrong or less Arguments given

Tool to work with FreeCAD Material definition cards

Examples:
  
   Material  "StandardMaterial/Steel.FCMat" 
 
Autor:
  (c) 2013 Juergen Riegel
  mail@juergen-riegel.net    
  Licence: LGPL

Version:
  0.1
"""

def importFCMat(fileName):
    "Read a FCMat file into a dictionary"
    import ConfigParser
    Config = ConfigParser.ConfigParser()
    Config.read(fileName)
    dict1 = {}
    for section in Config.sections():
        options = Config.options(section)
        for option in options:
            dict1[section+'_'+option] = Config.get(section, option)

    return dict1
    
def exportFCMat(fileName,matDict):
    "Write a material dictionary to a FCMat file"
    import ConfigParser,string
    Config = ConfigParser.ConfigParser()
    
    # create groups
    for x in matDict.keys():
        grp,key = string.split(x,sep='_')
        if not Config.has_section(grp):
            Config.add_section(grp)

    # fill groups
    for x in matDict.keys():
        grp,key = string.split(x,sep='_')
        Config.set(grp,key,matDict[x])
    
    Preamble = "# This is a FreeCAD material-card file\n\n"
    # Writing our configuration file to 'example.cfg'
    with open(fileName, 'wb') as configfile:
        configfile.write(Preamble)
        Config.write(configfile)

    
    
    
    
if __name__ == '__main__':
    import sys, getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], "c:", ["output-csv="])
    except getopt.GetoptError:
        # print help information and exit:
        sys.stderr.write(CommandlineUsage)
        sys.exit(1)

    # checking on the options
    for o, a in opts:
        if o in ("-c", "--output-csv"):
            print "writing file: " + a +"\n"
            OutPath = a

    # runing through the files
    FileName = args[0]

    kv_map = importFCMat(FileName)
    for k in kv_map.keys():
        print `k` + " : " + `kv_map[k]`
    sys.exit(0) # no error
    


