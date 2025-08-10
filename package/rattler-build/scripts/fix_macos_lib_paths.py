import os
import subprocess
import re
import sys

if len(sys.argv) < 1 or "-h" in sys.argv:
    print("""Usage: python fix_macos_paths.py <scan_path> [-r] [-s]

          Options:
            -r      scan the directory recursively
            -s      scan only without fixing absolute paths in LC_RPATH or LC_REEXPORT_DYLIB
          """)
    sys.exit(1)

scan_path = os.path.abspath(os.path.expanduser(sys.argv[1]))
recursive = "-r" in sys.argv
scanmode = "-s" in sys.argv

def get_lc_paths(output):
    if "is not an object file" in output:
        return [], []

    rpath_result = []
    reexport_result = []

    matches = re.finditer(r'cmd LC_RPATH', output)
    for match in matches:
        pos = match.start(0)

        path_match = re.search(r'path (.*) \(offset.+?\)', output[pos:])
        rpath_result.append(path_match.group(1))

    matches = re.finditer(r'cmd LC_REEXPORT_DYLIB', output)
    for match in matches:
        pos = match.start(0)

        path_match = re.search(r'name (.*) \(offset.+?\)', output[pos:])
        reexport_result.append(path_match.group(1))

    return rpath_result, reexport_result

def remove_rpath(file_path, rpath):
    subprocess.run(['install_name_tool', '-delete_rpath', rpath, file_path])
    subprocess.run(['codesign', '--force', '--sign', '-', file_path])
    print(f'\nRemoved rpath {rpath} from {file_path}')

def change_reexport_dylib(file_path, reexport_dylib):
    rel_reexport_dylib = "@rpath/" + os.path.basename(reexport_dylib)
    subprocess.run(['install_name_tool', '-change', reexport_dylib, rel_reexport_dylib, file_path])
    subprocess.run(['codesign', '--force', '--sign', '-', file_path])
    print(f'\nChanged reexport dylib {reexport_dylib} to {rel_reexport_dylib} in {file_path}')

def scan_directory(directory, recursive=False):
    if recursive:
        print(f"Recursively scanning dir: {directory}")
    else:
        print(f"Scanning dir: {directory}")

    for filename in os.listdir(directory):
        full_path = os.path.join(directory, filename)
        if recursive and os.path.isdir(full_path):
            scan_directory(full_path, recursive)
            continue
        elif not os.path.isfile(full_path) or os.path.islink(full_path):
            continue

        try:
            output = subprocess.check_output(['otool', '-l', full_path], text=True)
            rpaths, reexport_dylibs = get_lc_paths(output)
        except:
            continue

        file_dir = os.path.dirname(full_path)
        rpaths_processed = set()
        for rpath in rpaths:
            if rpath == "@loader_path":
                rpath = "@loader_path/"

            if os.path.isabs(rpath) and os.path.samefile(file_dir, rpath):
                if scanmode:
                    print(f'\nFound absolute path in LC_RPATH: {rpath}\nIn: {full_path}')
                else:
                    remove_rpath(full_path, rpath)

            if rpath in rpaths_processed:
                if scanmode:
                    print(f'\nFound duplicate RPATH: {rpath}\nIn: {full_path}')
                else:
                    remove_rpath(full_path, rpath)
            rpaths_processed.add(rpath)
        for reexport_dylib in reexport_dylibs:
            if os.path.isabs(reexport_dylib):
                if scanmode:
                    print(f'\nFound absolute path in LC_REEXPORT_DYLIB: {reexport_dylib}\nIn: {full_path}')
                else:
                    change_reexport_dylib(full_path, reexport_dylib)

scan_directory(scan_path, recursive)
print("Done")
