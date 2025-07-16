# pylint: disable=line-too-long
"""
 QT Pretty Printer for LLDB

 Currently supports displaying the following QT types in the debugger
     * QString


 Can be used from LLDB via:

 (lldb) command script import {Your FreeCAD source dir}/contrib/debugger/qt_pretty_printers_lldb.py

 (lldb) frame variable test
 (QString) test = "This is a test string."
 (lldb)


 Can be used in VSCode via launch.json (tested with CodeLLDB extension)

 {
     "version": "0.2.0",
     "configurations": [
         {
             "type": "lldb",
             "request": "launch",
             "name": "Debug LLDB",
             "stopOnEntry": false,
             "program": "${workspaceFolder}/build/bin/FreeCAD",
             "args": [],
             "cwd": "${workspaceFolder}/build/",
             "preLaunchTask": "CMake: build",
             "preRunCommands": [
                 "command script import ${workspaceFolder}/contrib/debugger/qt_pretty_printers_lldb.py"
             ]
         }
     ]
 }


 Note: There may be variances between different operating systems and/or build configurations
       in how QString data is stored internally.  This was tested on debian12/amd64 and
       macOS/arm64. @BootsSiR
"""
# pylint: enable=line-too-long

import lldb

def QStringPrinter(val, _internal_dict):
    """Pretty-printer for QString objects in LLDB."""
    try:
        # Access the 'd' member of QString
        d = val.GetChildMemberWithName("d")
        if not d.IsValid():
            return 'null'

        # some machines have a different QString object layout (macOS? arm64?) so
        # first we must check if a ptr member exists
        ptr_field = d.GetChildMemberWithName("ptr")

        # if the ptr member exists, we will use that as the base address for our text data.
        if ptr_field.IsValid():
            # use the ptr value
            data_base_address = ptr_field.GetValueAsUnsigned()
        else:
            # we don't have a ptr member, so let's use an offset from the address of the
            # QString::Data as our base address
            d_address = d.GetValueAsUnsigned()

            # Assume the string data starts immediately after the first 24 bytes
            data_base_address = d_address + 24

        # if at this point, we don't have a valid data base address
        # we cannot proceed
        if data_base_address == 0:
            return "null"

        # Initialize variables for memory reading
        process = val.GetTarget().GetProcess()
        error = lldb.SBError()
        chunk_size = 256  # Size of each memory chunk to read
        string_data = []
        offset = 0

        # Read memory in chunks until null-terminator is found
        while True:
            raw_data = process.ReadMemory(data_base_address + offset, chunk_size, error)
            if not error.Success():
                return f"<error: unable to read memory: {error.GetCString()}>"

            # Parse UTF-16 data from the chunk
            for i in range(0, len(raw_data), 2):  # char16_t is 2 bytes
                char16_val = int.from_bytes(raw_data[i:i+2], byteorder='little')
                if char16_val == 0:  # Null-terminator found
                    return f'"{ "".join(string_data) }"'
                string_data.append(chr(char16_val))

            # Increase the offset for the next chunk
            offset += chunk_size

    except Exception as e: # pylint: disable=broad-except
        return f"<error: {str(e)}>"

def __lldb_init_module(debugger, _internal_dict):
    """Register the QString pretty-printer with LLDB."""
    print("QT pretty-printer script loaded.")
    debugger.HandleCommand('type summary add QString -F qt_pretty_printers_lldb.QStringPrinter')
    print("QT pretty-printer registered.")
