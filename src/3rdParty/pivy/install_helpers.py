import os
import sys
import shutil


PIVY_HEADER = """\
#ifdef __PIVY__
%%include %s
#endif

"""


def swigify_header(header_file, include_file):
    sys.stdout.write("create swigified header: " + header_file + "\n")

    fd = open(header_file, "r+")
    contents = fd.readlines()

    ins_line_nr = -1
    for line in contents:
        ins_line_nr += 1
        if line.find("#include ") != -1:
            break

    if ins_line_nr != -1:
        contents.insert(ins_line_nr, PIVY_HEADER % (include_file))
        fd.seek(0)
        fd.writelines(contents)
    else:
        print("[failed]")
        sys.exit(1)
    fd.close


def copy_and_swigify_header(interface_dir, include_dir, fname):
    """Copy the header file to the local include directory. Add an
    #include line at the beginning for the SWIG interface file..."""

    if fname.endswith(".i"):  # consider ".i" files
        fname_h = fname[:-2] + ".h"  # corresponding ".h" file
        from_file = os.path.join(include_dir, fname_h)
        to_file = os.path.join(interface_dir, fname_h)

    elif fname.endswith(".fix"):  # just drop the suffix
        # fixes for SWIG 1.3.21 and upwards
        # (mostly workarounding swig's preprocessor "function like macros"
        # preprocessor bug when no parameters are provided which then results
        # in no constructors being created in the wrapper)
        fname_nosuffix = fname[:-4]
        from_file = os.path.join(interface_dir, fname)
        to_file = os.path.join(interface_dir, fname_nosuffix)

    elif sys.platform == "win32" and fname.endswith(".win32"):  # just drop the suffix
        # had to introduce this because windows is a piece of crap
        fname_nosuffix = fname[:-6]
        from_file = os.path.join(interface_dir, fname)
        to_file = os.path.join(interface_dir, fname_nosuffix)

    else:  # ignore other extensions
        return

    if not os.path.isfile(os.path.join(from_file)):
        return

    # copy
    shutil.copyfile(from_file, to_file)

    # and swigify
    if fname.endswith(".i"):  # consider ".i" files
        swigify_header(to_file, fname)


def swigify(interface_dir, include_dir, component="Inventor"):
    """Prepare header files for SWIG"""

    # find files within interface_dir/component
    interface_walker = os.walk(os.path.join(interface_dir, component))
    for dirpath, _, fnames in interface_walker:
        for fname in fnames:
            # only the filename relative to below interface_dir is needed
            relative_fname = os.path.join(dirpath[1+len(interface_dir):], fname)
            copy_and_swigify_header(interface_dir, include_dir, relative_fname)


if __name__ == "__main__":
    swigify(sys.argv[1], sys.argv[2])
