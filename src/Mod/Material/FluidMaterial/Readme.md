# FreeCAD fluid material library

 It's intended to gather the most common fluid properties, water, air, which are useful for other modules and workbenches.

## User defined material

To prevent the database from becoming inefficiently large it is only limited to commonly used variables at 20 degrees Celsius at 1 atm.  

Users can defined new material, either in Fem material card editor, or directly generate textual material file, * .FCMat, see example in this folder.   

To enable new material, go to FreeCAD menu "Edit->Preference..." Cfd preference page (select on the left panel) and switch to materiai tab on the right. 

Browse to your material folder, and save/apply this preference, new material will be Material with same name as FreeCAD material has higher priority,

so user defined` Water` material will not appear in Fem material task panel's dropbox list, just give it a different name!

### Edit material value

Please verify the fluid material properties before use. It aims to serve as a quick reference and does not aim to be an extended look up table.



## Add new material to Material module

1. follow examples in material folders to create new material file
2.  stick to the meta data definition in `src/Mod/Material/Templatematerial.yml` for property name
3. add the file name into the `src/Mod/Material/CMakeLists.txt` , so the new files can be installed to the properly place during compiling and installation. 

## Change log

CfdOF module authored 5 material types, values are taken from FM White (2011) Fluid Mechanics. 



Currently, 3 (Water, Air, None) are merged into Fem module and maintained by Cfd module author, Qingfeng Xia 

