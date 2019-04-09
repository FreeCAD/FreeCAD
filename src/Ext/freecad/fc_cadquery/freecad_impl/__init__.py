"""
    Copyright (C) 2011-2015  Parametric Products Intellectual Holdings, LLC

    This file is part of CadQuery.

    CadQuery is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    CadQuery is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; If not, see <http://www.gnu.org/licenses/>
"""
import os
import sys
import glob


#FreeCAD has crudified the stdout stream with a bunch of STEP output
#garbage
#this cleans it up
#so it is possible to use stdout for object output
class suppress_stdout_stderr(object):
    '''
    A context manager for doing a "deep suppression" of stdout and stderr in
    Python, i.e. will suppress all print, even if the print originates in a
    compiled C/Fortran sub-function.
       This will not suppress raised exceptions, since exceptions are printed
    to stderr just before a script exits, and after the context manager has
    exited (at least, I think that is why it lets exceptions through).

    '''
    def __init__(self):
        # Open a pair of null files
        self.null_fds =  [os.open(os.devnull,os.O_RDWR) for x in range(2)]
        # Save the actual stdout (1) and stderr (2) file descriptors.
        self.save_fds = [os.dup(1), os.dup(2)]

    def __enter__(self):
        # Assign the null pointers to stdout and stderr.
        os.dup2(self.null_fds[0],1)
        os.dup2(self.null_fds[1],2)

    def __exit__(self, *_):
        # Re-assign the real stdout/stderr back to (1) and (2)
        os.dup2(self.save_fds[0],1)
        os.dup2(self.save_fds[1],2)
        # Close all file descriptors
        for fd in self.null_fds + self.save_fds:
            os.close(fd)

def _fc_path():
    """Find FreeCAD"""
    # Look for FREECAD_LIB env variable
    _PATH = os.environ.get('FREECAD_LIB', '')
    if _PATH and os.path.exists(_PATH):
        return _PATH

    # Try to guess if using Anaconda
    if 'env' in sys.prefix:
        if sys.platform.startswith('linux') or sys.platform.startswith('darwin'):
            _PATH = os.path.join(sys.prefix,'lib')
            # return PATH if FreeCAD.[so,pyd] is present
            if len(glob.glob(os.path.join(_PATH,'FreeCAD.so'))) > 0:
                return _PATH
        elif sys.platform.startswith('win'):
            _PATH = os.path.join(sys.prefix,'Library','bin')
            # return PATH if FreeCAD.[so,pyd] is present
            if len(glob.glob(os.path.join(_PATH,'FreeCAD.pyd'))) > 0:
                return _PATH

    if sys.platform.startswith('linux'):
        # Make some dangerous assumptions...
        for _PATH in [
                os.path.join(os.path.expanduser("~"), "lib/freecad/lib"),
                "/usr/local/lib/freecad/lib",
                "/usr/lib/freecad/lib",
                "/opt/freecad/lib/",
                "/usr/bin/freecad/lib",
                "/usr/lib/freecad-daily/lib",
                "/usr/lib/freecad",
                "/usr/lib64/freecad/lib",
                ]:
            if os.path.exists(_PATH):
                return _PATH

    elif sys.platform.startswith('win'):
        # Try all the usual suspects
        for _PATH in [
                "c:/Program Files/FreeCAD0.12/bin",
                "c:/Program Files/FreeCAD0.13/bin",
                "c:/Program Files/FreeCAD0.14/bin",
                "c:/Program Files/FreeCAD0.15/bin",
                "c:/Program Files/FreeCAD0.16/bin",
                "c:/Program Files/FreeCAD0.17/bin",
                "c:/Program Files (x86)/FreeCAD0.12/bin",
                "c:/Program Files (x86)/FreeCAD0.13/bin",
                "c:/Program Files (x86)/FreeCAD0.14/bin",
                "c:/Program Files (x86)/FreeCAD0.15/bin",
                "c:/Program Files (x86)/FreeCAD0.16/bin",
                "c:/Program Files (x86)/FreeCAD0.17/bin",
                "c:/apps/FreeCAD0.12/bin",
                "c:/apps/FreeCAD0.13/bin",
                "c:/apps/FreeCAD0.14/bin",
                "c:/apps/FreeCAD0.15/bin",
                "c:/apps/FreeCAD0.16/bin",
                "c:/apps/FreeCAD0.17/bin",
                "c:/Program Files/FreeCAD 0.12/bin",
                "c:/Program Files/FreeCAD 0.13/bin",
                "c:/Program Files/FreeCAD 0.14/bin",
                "c:/Program Files/FreeCAD 0.15/bin",
                "c:/Program Files/FreeCAD 0.16/bin",
                "c:/Program Files/FreeCAD 0.17/bin",
                "c:/Program Files (x86)/FreeCAD 0.12/bin",
                "c:/Program Files (x86)/FreeCAD 0.13/bin",
                "c:/Program Files (x86)/FreeCAD 0.14/bin",
                "c:/Program Files (x86)/FreeCAD 0.15/bin",
                "c:/Program Files (x86)/FreeCAD 0.16/bin",
                "c:/Program Files (x86)/FreeCAD 0.17/bin",
                "c:/apps/FreeCAD 0.12/bin",
                "c:/apps/FreeCAD 0.13/bin",
                "c:/apps/FreeCAD 0.14/bin",
                "c:/apps/FreeCAD 0.15/bin",
                "c:/apps/FreeCAD 0.16/bin",
                "c:/apps/FreeCAD 0.17/bin",
                ]:
            if os.path.exists(_PATH):
                return _PATH

    elif sys.platform.startswith('darwin'):
        # Assume we're dealing with a Mac
        for _PATH in [
                "/Applications/FreeCAD.app/Contents/lib",
                os.path.join(os.path.expanduser("~"),
                             "Library/Application Support/FreeCAD/lib"),
                ]:
            if os.path.exists(_PATH):
                return _PATH

    raise ImportError('cadquery was unable to determine freecad library path')


# Make sure that the correct FreeCAD path shows up in Python's system path
with suppress_stdout_stderr():
    try:
        import FreeCAD
    except ImportError:
        path = _fc_path()
        sys.path.insert(0, path)
        import FreeCAD

# logging
from . import console_logging
