# pylint: disable=line-too-long
"""
 QT Pretty Printer for GDB

 Currently supports displaying the following QT types in the debugger
     * QString


 Can be used from GDB via:

 (gdb) python
 >import sys
 >sys.path.insert(0, '{Your FreeCAD source dir}/contrib/debugger')
 >from qt_pretty_printers_gdb import register_qt_printers
 >register_qt_printers()
 >end
 QT pretty-printer script loaded.
 QT pretty-printer registered.
 (gdb) run
 *breakpoint*
 (gdb) p testString
 $1 = This is a QString in the debugger!
 (gdb)



 Can be used in VSCode via launch.json

 {
     "version": "0.2.0",
     "configurations": [
         {
             "name": "(gdb) Launch",
             "type": "cppdbg",
             "request": "launch",
             "program": "${workspaceFolder}/build/bin/FreeCAD",
             "args": [],
             "stopAtEntry": true,
             "cwd": "${workspaceFolder}/build",
             "environment": [],
             "externalConsole": false,
             "MIMode": "gdb",
             "setupCommands": [
                 {
                     "description": "Load QT pretty-printers for GDB",
                     "text": "python import sys; sys.path.append('${workspaceFolder}/contrib/debugger'); from qt_pretty_printers_gdb import register_qt_printers; register_qt_printers()",
                     "ignoreFailures": false
                 },
                 {
                     "description": "Enable pretty-printing for gdb",
                     "text": "-enable-pretty-printing",
                     "ignoreFailures": true
                 },
                 {
                     "description": "Set Disassembly Flavor to Intel",
                     "text": "-gdb-set disassembly-flavor intel",
                     "ignoreFailures": true
                 }
             ],
             "miDebuggerPath": "/usr/bin/gdb"
         }
     ]
 }


 Note: There may be variances between different operating systems and/or build configurations
       in how QString data is stored internally.  This was tested on debian12/amd64. @BootsSiR
"""
# pylint: enable=line-too-long

import gdb

class QStringPrinter:
    """Pretty-printer class for QString objects in GDB."""
    def __init__(self, val):
        self.val = val

    def to_string(self):
        """Pretty-printer function for QString objects in LLDB."""
        try:
            d = self.val["d"]
            if d == 0:
                return 'null'

            offset = int(d["offset"])
            data_ptr = d.cast(gdb.lookup_type("char").pointer()) + offset
            char16_t_ptr = data_ptr.cast(gdb.lookup_type("char16_t").pointer())

            string_data = []
            i = 0
            while char16_t_ptr[i] != 0:
                string_data.append(chr(char16_t_ptr[i]))
                i += 1

            return "".join(string_data)
        except gdb.error as e:
            if "Cannot access memory at address" in str(e):
                return "<uninitialized>"
            return f"<error: {str(e)}>"
        except Exception as e: # pylint: disable=broad-except
            return f"<unexpected error: {str(e)}>"


def lookup_function(val):
    """Maps a type to the appropriate pretty printer"""
    try:
        qstring_type = gdb.lookup_type("QString")
        if val.type.strip_typedefs() == qstring_type:
            return QStringPrinter(val)
        return None
    except gdb.error:
        return None


def register_qt_printers(objfile=None):
    """Register the QString pretty-printer with GDB."""
    if objfile is None:
        objfile = gdb.current_objfile()

    # Check if the printer is already registered
    if not any(printer == lookup_function for printer in gdb.pretty_printers): # pylint: disable=comparison-with-callable
        print("QT pretty-printer script loaded.")
        gdb.pretty_printers.append(lookup_function)
        print("QT pretty-printer registered.")