#***************************************************************************
#*   Copyright (c) 2001,2002 Jürgen Riegel <juergen.riegel@web.de>         *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software  you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation  either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY  without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD  if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

# FreeCAD init module
#
# Gathering all the information to start FreeCAD.
# This is the second of three init scripts.
# The third one runs when the gui is up,

# imports the one and only
import FreeCAD

def removeFromPath(module_name):
    """removes the module from the sys.path. The entry point for imports
        will therefore always be FreeCAD.
        eg.: from FreeCAD.Module.submodule import function"""
    import sys
    paths = sys.path
    for path in paths:
        if module_name in path:
            sys.path.remove(path)
            return
    Wrn(module_name + " not found in sys.path\n")

def setupSearchPaths(PathExtension):
    # DLL resolution in Python 3.8 on Windows has changed
    import sys
    if sys.platform == 'win32' and hasattr(os, "add_dll_directory"):
        if "FREECAD_LIBPACK_BIN" in os.environ:
            os.add_dll_directory(os.environ["FREECAD_LIBPACK_BIN"])
        if "WINDIR" in os.environ:
            os.add_dll_directory(os.environ["WINDIR"] + os.sep + "system32")
        for path in PathExtension:
            os.add_dll_directory(path)

    PathEnvironment = PathExtension.pop(0) + os.pathsep
    for path in PathExtension:
        try:
            PathEnvironment += path + os.pathsep
        except UnicodeDecodeError:
            Wrn('Filter invalid module path: u{}\n'.format(repr(path)))

    # new paths must be prepended to avoid to load a wrong version of a library
    try:
        os.environ["PATH"] = PathEnvironment + os.environ["PATH"]
    except UnicodeEncodeError:
        Log('UnicodeEncodeError was raised when concatenating unicode string with PATH. Try to replace non-ascii chars...\n')
        os.environ["PATH"] = PathEnvironment.encode(errors='replace') + os.environ["PATH"]
        Log('done\n')
    except KeyError:
        os.environ["PATH"] = PathEnvironment

FreeCAD._importFromFreeCAD = removeFromPath


def InitApplications():
    # Checking on FreeCAD module path ++++++++++++++++++++++++++++++++++++++++++
    ModDir = FreeCAD.getHomePath()+'Mod'
    ModDir = os.path.realpath(ModDir)
    ExtDir = FreeCAD.getHomePath()+'Ext'
    ExtDir = os.path.realpath(ExtDir)
    BinDir = FreeCAD.getHomePath()+'bin'
    BinDir = os.path.realpath(BinDir)
    libpaths = []
    LibDir = FreeCAD.getHomePath()+'lib'
    LibDir = os.path.realpath(LibDir)
    if os.path.exists(LibDir):
        libpaths.append(LibDir)
    Lib64Dir = FreeCAD.getHomePath()+'lib64'
    Lib64Dir = os.path.realpath(Lib64Dir)
    if os.path.exists(Lib64Dir):
        libpaths.append(Lib64Dir)
    LibPyDir = FreeCAD.getHomePath()+'lib-py3'
    LibPyDir = os.path.realpath(LibPyDir)
    if (os.path.exists(LibPyDir)):
        libpaths.append(LibPyDir)
    LibFcDir = FreeCAD.getLibraryDir()
    LibFcDir = os.path.realpath(LibFcDir)
    if (os.path.exists(LibFcDir) and not LibFcDir in libpaths):
        libpaths.append(LibFcDir)
    AddPath = FreeCAD.ConfigGet("AdditionalModulePaths").split(";")
    HomeMod = FreeCAD.getUserAppDataDir()+"Mod"
    HomeMod = os.path.realpath(HomeMod)
    MacroStd = App.getUserMacroDir(False)
    MacroDir = App.getUserMacroDir(True)
    MacroMod = os.path.realpath(MacroDir+"/Mod")
    SystemWideMacroDir = FreeCAD.getHomePath()+'Macro'
    SystemWideMacroDir = os.path.realpath(SystemWideMacroDir)

    #print FreeCAD.getHomePath()
    if os.path.isdir(FreeCAD.getHomePath()+'src\\Tools'):
        sys.path.append(FreeCAD.getHomePath()+'src\\Tools')



    # Searching for module dirs +++++++++++++++++++++++++++++++++++++++++++++++++++
    # Use dict to handle duplicated module names
    ModDict = {}
    if os.path.isdir(ModDir):
        ModDirs = os.listdir(ModDir)
        for i in ModDirs: ModDict[i.lower()] = os.path.join(ModDir,i)
    else:
        Wrn ("No modules found in " + ModDir + "\n")
    # Search for additional modules in the home directory
    if os.path.isdir(HomeMod):
        HomeMods = os.listdir(HomeMod)
        for i in HomeMods: ModDict[i.lower()] = os.path.join(HomeMod,i)
    elif os.path.isdir(os.path.join(os.path.expanduser("~"),".FreeCAD","Mod")):
        # Check if old location exists
        Wrn ("User path has changed to " + FreeCAD.getUserAppDataDir() + ". Please move user modules and macros\n")
    # Search for additional modules in the macro directory
    if os.path.isdir(MacroMod):
        MacroMods = os.listdir(MacroMod)
        for i in MacroMods:
            key = i.lower()
            if key not in ModDict: ModDict[key] = os.path.join(MacroMod,i)
    # Search for additional modules in command line
    for i in AddPath:
        if os.path.isdir(i): ModDict[i] = i
    #AddModPaths = App.ParamGet("System parameter:AdditionalModulePaths")
    #Err( AddModPaths)
    # add also this path so that all modules search for libraries
    # they depend on first here
    PathExtension = []
    PathExtension.append(BinDir)

    # prepend all module paths to Python search path
    Log('Init:   Searching for modules...\n')


    # to have all the module-paths available in FreeCADGuiInit.py:
    FreeCAD.__ModDirs__ = list(ModDict.values())

    # this allows importing with:
    # from FreeCAD.Module import package
    FreeCAD.__path__ = [ModDir] + libpaths + [HomeMod]

    # also add these directories to the sys.path to
    # not change the old behaviour. once we have moved to
    # proper python modules this can eventually be removed.
    sys.path = [ModDir] + libpaths + [ExtDir] + sys.path

    # The AddonManager may install additional Python packages in
    # these paths:
    import platform
    major,minor,_ = platform.python_version_tuple()
    vendor_path = os.path.join(
        FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages",f"py{major}{minor}"
    )
    if os.path.isdir(vendor_path):
        sys.path.append(vendor_path)

    additional_packages_path = os.path.join(FreeCAD.getUserAppDataDir(),"AdditionalPythonPackages")
    if os.path.isdir(additional_packages_path):
        sys.path.append(additional_packages_path)

    def RunInitPy(Dir):
        InstallFile = os.path.join(Dir,"Init.py")
        if (os.path.exists(InstallFile)):
            try:
                with open(InstallFile, 'rt', encoding='utf-8') as f:
                    exec(compile(f.read(), InstallFile, 'exec'))
            except Exception as inst:
                Log('Init:      Initializing ' + Dir + '... failed\n')
                Log('-'*100+'\n')
                Log(traceback.format_exc())
                Log('-'*100+'\n')
                Err('During initialization the error "' + str(inst) + '" occurred in ' + InstallFile + '\n')
                Err('Please look into the log file for further information\n')
            else:
                Log('Init:      Initializing ' + Dir + '... done\n')
        else:
            Log('Init:      Initializing ' + Dir + '(Init.py not found)... ignore\n')

    def processMetadataFile(MetadataFile):
        meta = FreeCAD.Metadata(MetadataFile)
        if not meta.supportsCurrentFreeCAD():
            Msg(f'NOTICE: {meta.Name} does not support this version of FreeCAD, so is being skipped\n')
            return None
        content = meta.Content
        if "workbench" in content:
            workbenches = content["workbench"]
            for workbench in workbenches:
                if not workbench.supportsCurrentFreeCAD():
                    Msg(f'NOTICE: {meta.Name} content item {workbench.Name} does not support this version of FreeCAD, so is being skipped\n')
                    return None
                subdirectory = workbench.Name if not workbench.Subdirectory else workbench.Subdirectory
                subdirectory = subdirectory.replace("/",os.path.sep)
                subdirectory = os.path.join(Dir, subdirectory)
                #classname = workbench.Classname
                sys.path.insert(0,subdirectory)
                PathExtension.append(subdirectory)
                RunInitPy(subdirectory)

    def tryProcessMetadataFile(MetadataFile):
        try:
            processMetadataFile(MetadataFile)
        except Exception as exc:
            Err(str(exc))

    for Dir in ModDict.values():
        if ((Dir != '') & (Dir != 'CVS') & (Dir != '__init__.py')):
            stopFile = os.path.join(Dir, "ADDON_DISABLED")
            if os.path.exists(stopFile):
                Msg(f'NOTICE: Addon "{Dir}" disabled by presence of ADDON_DISABLED stopfile\n')
                continue
            sys.path.insert(0,Dir)
            PathExtension.append(Dir)
            MetadataFile = os.path.join(Dir, "package.xml")
            if os.path.exists(MetadataFile):
                tryProcessMetadataFile(MetadataFile)
            else:
                RunInitPy(Dir)

    extension_modules = []

    try:
        import pkgutil
        import importlib
        import freecad
        for _, freecad_module_name, freecad_module_ispkg in pkgutil.iter_modules(freecad.__path__, "freecad."):
            if freecad_module_ispkg:
                Log('Init: Initializing ' + freecad_module_name + '\n')
                try:
                    # Check for a stopfile
                    stopFile = os.path.join(FreeCAD.getUserAppDataDir(), "Mod", freecad_module_name[8:], "ADDON_DISABLED")
                    if os.path.exists(stopFile):
                        Msg(f'NOTICE: Addon "{freecad_module_name}" disabled by presence of ADDON_DISABLED stopfile\n')
                        continue

                    # Make sure that package.xml (if present) does not exclude this version of FreeCAD
                    MetadataFile = os.path.join(FreeCAD.getUserAppDataDir(), "Mod", freecad_module_name[8:], "package.xml")
                    if os.path.exists(MetadataFile):
                        meta = FreeCAD.Metadata(MetadataFile)
                        if not meta.supportsCurrentFreeCAD():
                            Msg(f'NOTICE: Addon "{freecad_module_name}" does not support this version of FreeCAD, so is being skipped\n')
                            continue

                    freecad_module = importlib.import_module(freecad_module_name)
                    extension_modules += [freecad_module_name]
                    if any (module_name == 'init' for _, module_name, ispkg in pkgutil.iter_modules(freecad_module.__path__)):
                        importlib.import_module(freecad_module_name + '.init')
                        Log('Init: Initializing ' + freecad_module_name + '... done\n')
                    else:
                        Log('Init: No init module found in ' + freecad_module_name + ', skipping\n')
                except Exception as inst:
                    Err('During initialization the error "' + str(inst) + '" occurred in ' + freecad_module_name + '\n')
                    Err('-'*80+'\n')
                    Err(traceback.format_exc())
                    Err('-'*80+'\n')
                    Log('Init:      Initializing ' + freecad_module_name + '... failed\n')
                    Log('-'*80+'\n')
                    Log(traceback.format_exc())
                    Log('-'*80+'\n')
    except ImportError as inst:
        Err('During initialization the error "' + str(inst) + '" occurred\n')

    Log("Using "+ModDir+" as module path!\n")
    # In certain cases the PathExtension list can contain invalid strings. We concatenate them to a single string
    # but check that the output is a valid string
    setupSearchPaths(PathExtension)
    path = os.environ["PATH"].split(os.pathsep)
    Log("System path after init:\n")
    for i in path:
        Log("   " + i + "\n")
    # add MacroDir to path (RFE #0000504)
    sys.path.append(MacroStd)
    sys.path.append(MacroDir)
    # add SystemWideMacroDir to path
    sys.path.append(SystemWideMacroDir)
    # add special path for MacOSX (bug #0000307)
    import platform
    if len(platform.mac_ver()[0]) > 0:
        sys.path.append(os.path.expanduser('~/Library/Application Support/FreeCAD/Mod'))

# some often used shortcuts (for lazy people like me  ;-)
App = FreeCAD
Log = FreeCAD.Console.PrintLog
Msg = FreeCAD.Console.PrintMessage
Err = FreeCAD.Console.PrintError
Wrn = FreeCAD.Console.PrintWarning
Crt = FreeCAD.Console.PrintCritical
Ntf = FreeCAD.Console.PrintNotification
Tnf = FreeCAD.Console.PrintTranslatedNotification

#store the cmake variales
App.__cmake__ = cmake;

#store unit test names
App.__unit_test__ = []

Log ('Init: starting App::FreeCADInit.py\n')

try:
    import sys,os,traceback,inspect
    from datetime import datetime
except ImportError:
    FreeCAD.Console.PrintError("\n\nSeems the python standard libs are not installed, bailing out!\n\n")
    raise

class FCADLogger(object):
    '''Convenient class for tagged logging.

       Example usage:
           >>> logger = FreeCAD.Logger('MyModule')
           >>> logger.info('log test {}',1)
           24.36053 <MyModule> <input>(1): test log 1

       The default output format is:
           <timestamp> <tag> <source file>(line number): message

       The message is formatted using new style Python string formatting, e.g.
       'test {}'.format(1). It is strongly recommended to not directly use
       Python string formatting, but pass additional argument indirectly through
       various logger print function, because the logger can skip string
       evaluation in case the logging level is disabled. For more options,
       please consult the docstring of __init__(), catch() and report().

       To set/get logger level:
           >>> FreeCAD.setLogLevel('MyModule','Trace')
           >>> FreeCAD.getLogLevel('MyModule')
           4

        There are five predefined logger level, each corresponding to an integer
        value, as shown below together with the corresponding logger print
        method,
            0: Error, Logger.error()
            1: Warning, Logger.warn()
            2: Message, Logger.msg() or info()
            3: Log, Logger.log() or debug()
            4: Trace, Logger.trace()

        FreeCAD.setLogLevel() supports both text and integer value, which allows
        you to define your own levels. The level set is persisted to user
        configuration file.

        By default any tag has a log level of 2 for release, and 3 for debug
        build.
    '''

    _levels = { 'Error':0, 'error':0,
                'Warning':1, 'warn':1,
                'Message':2, 'msg':2, 'info':2,
                'Log':3, 'log':3, 'debug':3,
                'Trace':4, 'trace':4,}
    _printer = [
            FreeCAD.Console.PrintError,
            FreeCAD.Console.PrintWarning,
            FreeCAD.Console.PrintMessage,
            FreeCAD.Console.PrintLog,
            FreeCAD.Console.PrintLog ]

    def __init__(self, tag, **kargs):
        '''Construct a logger instance.

        Supported arguments are their default values are,

        * tag: a string tag for this logger. The log level of this logger can be
               accessed using FreeCAD.getLogLevel(tag)/setLogLevel(tag,level).
               All logger instance with the same tag shares the same level
               setting.

        * printTag (True): whether to print tag

        * noUpdateUI (True): whether to update GUI when printing. This is useful
                             to show log output on lengthy operations. Be
                             careful though, this may allow unexpected user
                             interaction when the application is busy, which may
                             lead to crash

        * timing (True): whether to print time stamp

        * lineno (True): whether to print source file and line number

        * parent (None): provide a parent logger, so that the log printing will
                         check for parent's log level in addition of its own

        * title ('FreeCAD'): message box title used by report()
        '''
        self.tag = tag
        self.laststamp = datetime.now()
        for key,default in (('printTag',True),('noUpdateUI',True),
                ('timing',True),('lineno',True),('parent',None),
                ('title','FreeCAD')) :
            setattr(self,key,kargs.get(key,default))

    def _isEnabledFor(self,level):
        '''Internal function to check for an integer log level.

            * level: integer log level
        '''

        if self.parent and not self.parent._isEnabledFor(level):
            return False
        return FreeCAD.getLogLevel(self.tag) >= level

    def isEnabledFor(self,level):
        '''To check for an integer or text log level.

            * level: integer or text log level
        '''
        if not isinstance(level,int):
            level = self.__class__._levels[level]
        return self._isEnabledFor(level)

    def error(self,msg,*args,**kargs):
        '''"Error" level log printer

            * msg: message string. May contain new style Python string formatter.

            This function accepts additional positional and keyword arguments,
            which are forward to string.format() to generate the logging
            message. It is strongly recommended to not directly use Python
            string formatting, but pass additional arguments here, because the
            printer can skip string evaluation in case the logging level is
            disabled.
        '''
        if self._isEnabledFor(0):
            frame = kargs.pop('frame',0)+1
            self._log(0,msg,frame,args,kargs)

    def warn(self,msg,*args,**kargs):
        '''"Warning" level log printer

            * msg: message string. May contain new style Python string formatter.

            This function accepts additional positional and keyword arguments,
            which are forward to string.format() to generate the logging
            message. It is strongly recommended to not directly use Python
            string formatting, but pass additional arguments here, because the
            printer can skip string evaluation in case the logging level is
            disabled.
        '''
        if self._isEnabledFor(1):
            frame = kargs.pop('frame',0)+1
            self._log(1,msg,frame,args,kargs)

    def msg(self,msg,*args,**kargs):
        '''"Message" level log printer

            * msg: message string. May contain new style Python string formatter.

            This function accepts additional positional and keyword arguments,
            which are forward to string.format() to generate the logging
            message. It is strongly recommended to not directly use Python
            string formatting, but pass additional arguments here, because the
            printer can skip string evaluation in case the logging level is
            disabled.
        '''
        if self._isEnabledFor(2):
            frame = kargs.pop('frame',0)+1
            self._log(2,msg,frame,args,kargs)

    info = msg

    def log(self,msg,*args,**kargs):
        '''"Log" level log printer

            * msg: message string. May contain new style Python string formatter.

            This function accepts additional positional and keyword arguments,
            which are forward to string.format() to generate the logging
            message. It is strongly recommended to not directly use Python
            string formatting, but pass additional arguments here, because the
            printer can skip string evaluation in case the logging level is
            disabled.
        '''
        if self._isEnabledFor(3):
            frame = kargs.pop('frame',0)+1
            self._log(3,msg,frame,args,kargs)

    debug = log

    def trace(self,msg,*args,**kargs):
        '''"Trace" level log printer

            * msg: message string. May contain new style Python string formatter.

            This function accepts additional positional and keyword arguments,
            which are forward to string.format() to generate the logging
            message. It is strongly recommended to not directly use Python
            string formatting, but pass additional arguments here, because the
            printer can skip string evaluation in case the logging level is
            disabled.
        '''
        if self._isEnabledFor(4):
            frame = kargs.pop('frame',0)+1
            self._log(4,msg,frame,args,kargs)

    def _log(self,level,msg,frame=0,args=(),kargs=None):
        '''Internal log printing function.

            * level: integer log level

            * msg: message, may contain new style string format specifier

            * frame (0): the calling frame for printing source file and line
                         number.  For example, in case you have your own logging
                         function, and you want to show the callers source
                         location, then set frame to one.

            * args: tuple for positional arguments to be passed to
                    string.format()

            * kargs: dictionary for keyword arguments to be passed to
                     string.format()
        '''

        if (args or kargs) and isinstance(msg,str):
            if not kargs:
                msg = msg.format(*args)
            else:
                msg = msg.format(*args,**kargs)

        prefix = ''

        if self.timing:
            now = datetime.now()
            prefix += '{} '.format((now-self.laststamp).total_seconds())
            self.laststamp = now

        if self.printTag:
            prefix += '<{}> '.format(self.tag)

        if self.lineno:
            try:
                frame = sys._getframe(frame+1)
                prefix += '{}({}): '.format(os.path.basename(
                    frame.f_code.co_filename),frame.f_lineno)
            except Exception:
                frame = inspect.stack()[frame+1]
                prefix += '{}({}): '.format(os.path.basename(frame[1]),frame[2])

        self.__class__._printer[level]('{}{}\n'.format(prefix,msg))

        if not self.noUpdateUI and FreeCAD.GuiUp:
            import FreeCADGui
            try:
                FreeCADGui.updateGui()
            except Exception:
                pass

    def _catch(self,level,msg,func,args=None,kargs=None):
        '''Internal function to log exception of any callable.

            * level: integer log level

            * msg: message string. Unlike _log(), this argument must not contain
                   any string formatter.

            * func: a callable object

            * args: tuple of positional arguments to be passed to func.

            * kargs: dictionary of keyword arguments to be passed to func.
        '''
        try:
            if not args:
                args = []
            if not kargs:
                kargs = {}
            return func(*args,**kargs)
        except Exception:
            if self._isEnabledFor(level):
                self._log(level,msg+'\n'+traceback.format_exc(),frame=2)

    def catch(self,msg,func,*args,**kargs):
        '''Catch any exception from a function and print as "Error".

            * msg: message string. Unlike log printer, this argument must not
                   contain any string formatter.

            * func: a callable object

            * args: tuple of positional arguments to be passed to func.

            * kargs: dictionary of keyword arguments to be passed to func.
        '''
        return self._catch(0,msg,func,args,kargs)

    def catchWarn(self,msg,func,*args,**kargs):
        '''Catch any exception from a function and print as "Warning".

            * msg: message string. Unlike log printer, this argument must not
                   contain any string formatter.

            * func: a callable object

            * args: tuple of positional arguments to be passed to func.

            * kargs: dictionary of keyword arguments to be passed to func.
        '''
        return self._catch(1,msg,func,args,kargs)

    def catchMsg(self,msg,func,*args,**kargs):
        '''Catch any exception from a function and print as "Message".

            * msg: message string. Unlike log printer, this argument must not
                   contain any string formatter.

            * func: a callable object

            * args: tuple of positional arguments to be passed to func.

            * kargs: dictionary of keyword arguments to be passed to func.
        '''
        return self._catch(2,msg,func,args,kargs)

    catchInfo = catchMsg

    def catchLog(self,msg,func,*args,**kargs):
        '''Catch any exception from a function and print as "Log".

            * msg: message string. Unlike log printer, this argument must not
                   contain any string formatter.

            * func: a callable object

            * args: tuple of positional arguments to be passed to func.

            * kargs: dictionary of keyword arguments to be passed to func.
        '''
        return self._catch(3,msg,func,args,kargs)

    catchDebug = catchLog

    def catchTrace(self,msg,func,*args,**kargs):
        '''Catch any exception from a function and print as "Trace".

            * msg: message string. Unlike log printer, this argument must not
                   contain any string formatter.

            * func: a callable object

            * args: tuple of positional arguments to be passed to func.

            * kargs: dictionary of keyword arguments to be passed to func.
        '''
        return self._catch(4,msg,func,args,kargs)

    def report(self,msg,func,*args,**kargs):
        '''Catch any exception report it with a message box.

            * msg: message string. Unlike log printer, this argument must not
                   contain any string formatter.

            * func: a callable object

            * args: tuple of positional arguments to be passed to func.

            * kargs: dictionary of keyword arguments to be passed to func.
        '''
        try:
            return func(*args,**kargs)
        except Exception as e:
            self.error(msg+'\n'+traceback.format_exc(),frame=1)
            if FreeCAD.GuiUp:
                import FreeCADGui,PySide
                PySide.QtGui.QMessageBox.critical(
                        FreeCADGui.getMainWindow(),self.title,str(e))

FreeCAD.Logger = FCADLogger

# init every application by importing Init.py
try:
    InitApplications()
except Exception as e:
    Err('Error in InitApplications ' + str(e) + '\n')
    Err('-'*80+'\n')
    Err(traceback.format_exc())
    Err('-'*80+'\n')

FreeCAD.addImportType("FreeCAD document (*.FCStd)","FreeCAD")

# set to no gui, is overwritten by InitGui
App.GuiUp = 0

# fill up unit definitions

App.Units.NanoMetre     = App.Units.Quantity('nm')
App.Units.MicroMetre    = App.Units.Quantity('um')
App.Units.MilliMetre    = App.Units.Quantity('mm')
App.Units.CentiMetre    = App.Units.Quantity('cm')
App.Units.DeciMetre     = App.Units.Quantity('dm')
App.Units.Metre         = App.Units.Quantity('m')
App.Units.KiloMetre     = App.Units.Quantity('km')

App.Units.MilliLiter    = App.Units.Quantity('ml')
App.Units.Liter         = App.Units.Quantity('l')

App.Units.Hertz         = App.Units.Quantity('Hz')
App.Units.KiloHertz     = App.Units.Quantity('kHz')
App.Units.MegaHertz     = App.Units.Quantity('MHz')
App.Units.GigaHertz     = App.Units.Quantity('GHz')
App.Units.TeraHertz     = App.Units.Quantity('THz')

App.Units.MicroGram     = App.Units.Quantity('ug')
App.Units.MilliGram     = App.Units.Quantity('mg')
App.Units.Gram          = App.Units.Quantity('g')
App.Units.KiloGram      = App.Units.Quantity('kg')
App.Units.Ton           = App.Units.Quantity('t')

App.Units.Second        = App.Units.Quantity('s')
App.Units.Minute        = App.Units.Quantity('min')
App.Units.Hour          = App.Units.Quantity('h')

App.Units.Ampere        = App.Units.Quantity('A')
App.Units.MilliAmpere   = App.Units.Quantity('mA')
App.Units.KiloAmpere    = App.Units.Quantity('kA')
App.Units.MegaAmpere    = App.Units.Quantity('MA')

App.Units.Kelvin        = App.Units.Quantity('K')
App.Units.MilliKelvin   = App.Units.Quantity('mK')
App.Units.MicroKelvin   = App.Units.Quantity('uK')

App.Units.MilliMole     = App.Units.Quantity('mmol')
App.Units.Mole          = App.Units.Quantity('mol')

App.Units.Candela       = App.Units.Quantity('cd')

App.Units.Inch          = App.Units.Quantity('in')
App.Units.Foot          = App.Units.Quantity('ft')
App.Units.Thou          = App.Units.Quantity('thou')
App.Units.Yard          = App.Units.Quantity('yd')
App.Units.Mile          = App.Units.Quantity('mi')

App.Units.SquareFoot    = App.Units.Quantity('sqft')
App.Units.CubicFoot     = App.Units.Quantity('cft')

App.Units.Pound         = App.Units.Quantity('lb')
App.Units.Ounce         = App.Units.Quantity('oz')
App.Units.Stone         = App.Units.Quantity('st')
App.Units.Hundredweights= App.Units.Quantity('cwt')

App.Units.Newton        = App.Units.Quantity('N')
App.Units.MilliNewton   = App.Units.Quantity('mN')
App.Units.KiloNewton    = App.Units.Quantity('kN')
App.Units.MegaNewton    = App.Units.Quantity('MN')

App.Units.NewtonPerMeter        = App.Units.Quantity('N/m')
App.Units.MilliNewtonPerMeter   = App.Units.Quantity('mN/m')
App.Units.KiloNewtonPerMeter    = App.Units.Quantity('kN/m')
App.Units.MegaNewtonPerMeter    = App.Units.Quantity('MN/m')

App.Units.Pascal        = App.Units.Quantity('Pa')
App.Units.KiloPascal    = App.Units.Quantity('kPa')
App.Units.MegaPascal    = App.Units.Quantity('MPa')
App.Units.GigaPascal    = App.Units.Quantity('GPa')

App.Units.MilliBar      = App.Units.Quantity('mbar')
App.Units.Bar           = App.Units.Quantity('bar')

App.Units.PoundForce    = App.Units.Quantity('lbf')
App.Units.Torr          = App.Units.Quantity('Torr')
App.Units.mTorr         = App.Units.Quantity('mTorr')
App.Units.yTorr         = App.Units.Quantity('uTorr')

App.Units.PSI           = App.Units.Quantity('psi')
App.Units.KSI           = App.Units.Quantity('ksi')
App.Units.MPSI          = App.Units.Quantity('Mpsi')

App.Units.Watt          = App.Units.Quantity('W')
App.Units.MilliWatt     = App.Units.Quantity('mW')
App.Units.KiloWatt      = App.Units.Quantity('kW')
App.Units.VoltAmpere    = App.Units.Quantity('VA')

App.Units.Volt          = App.Units.Quantity('V')
App.Units.MilliVolt     = App.Units.Quantity('mV')
App.Units.KiloVolt      = App.Units.Quantity('kV')

App.Units.MegaSiemens   = App.Units.Quantity('MS')
App.Units.KiloSiemens   = App.Units.Quantity('kS')
App.Units.Siemens       = App.Units.Quantity('S')
App.Units.MilliSiemens  = App.Units.Quantity('mS')
App.Units.MicroSiemens  = App.Units.Quantity('uS')

App.Units.Ohm          = App.Units.Quantity('Ohm')
App.Units.KiloOhm      = App.Units.Quantity('kOhm')
App.Units.MegaOhm      = App.Units.Quantity('MOhm')

App.Units.Coulomb       = App.Units.Quantity('C')

App.Units.Tesla         = App.Units.Quantity('T')
App.Units.Gauss         = App.Units.Quantity('G')

App.Units.Weber         = App.Units.Quantity('Wb')

# disable Oersted because people need to input e.g. a field strength of
# 1 ampere per meter -> 1 A/m and not get the recalculation to Oersted
# App.Units.Oersted       = App.Units.Quantity('Oe')

App.Units.PicoFarad     = App.Units.Quantity('pF')
App.Units.NanoFarad     = App.Units.Quantity('nF')
App.Units.MicroFarad    = App.Units.Quantity('uF')
App.Units.MilliFarad    = App.Units.Quantity('mF')
App.Units.Farad         = App.Units.Quantity('F')

App.Units.NanoHenry     = App.Units.Quantity('nH')
App.Units.MicroHenry    = App.Units.Quantity('uH')
App.Units.MilliHenry    = App.Units.Quantity('mH')
App.Units.Henry         = App.Units.Quantity('H')

App.Units.Joule         = App.Units.Quantity('J')
App.Units.MilliJoule    = App.Units.Quantity('mJ')
App.Units.KiloJoule     = App.Units.Quantity('kJ')
App.Units.NewtonMeter   = App.Units.Quantity('Nm')
App.Units.VoltAmpereSecond   = App.Units.Quantity('VAs')
App.Units.WattSecond    = App.Units.Quantity('Ws')
App.Units.KiloWattHour  = App.Units.Quantity('kWh')
App.Units.ElectronVolt  = App.Units.Quantity('eV')
App.Units.KiloElectronVolt = App.Units.Quantity('keV')
App.Units.MegaElectronVolt = App.Units.Quantity('MeV')
App.Units.Calorie       = App.Units.Quantity('cal')
App.Units.KiloCalorie   = App.Units.Quantity('kcal')

App.Units.MPH           = App.Units.Quantity('mi/h')
App.Units.KMH           = App.Units.Quantity('km/h')

App.Units.Degree        = App.Units.Quantity('deg')
App.Units.Radian        = App.Units.Quantity('rad')
App.Units.Gon           = App.Units.Quantity('gon')
App.Units.AngularMinute = App.Units.Quantity().AngularMinute
App.Units.AngularSecond = App.Units.Quantity().AngularSecond


# SI base units
# (length, weight, time, current, temperature, amount of substance, luminous intensity, angle)
App.Units.AmountOfSubstance           = App.Units.Unit(0,0,0,0,0,1)
App.Units.ElectricCurrent             = App.Units.Unit(0,0,0,1)
App.Units.Length                      = App.Units.Unit(1)
App.Units.LuminousIntensity           = App.Units.Unit(0,0,0,0,0,0,1)
App.Units.Mass                        = App.Units.Unit(0,1)
App.Units.Temperature                 = App.Units.Unit(0,0,0,0,1)
App.Units.TimeSpan                    = App.Units.Unit(0,0,1)

# all other combined units
App.Units.Acceleration                = App.Units.Unit(1,0,-2)
App.Units.Angle                       = App.Units.Unit(0,0,0,0,0,0,0,1)
App.Units.AngleOfFriction             = App.Units.Unit(0,0,0,0,0,0,0,1)
App.Units.Area                        = App.Units.Unit(2)
App.Units.CompressiveStrength         = App.Units.Unit(-1,1,-2)
App.Units.CurrentDensity              = App.Units.Unit(-2,0,0,1)
App.Units.Density                     = App.Units.Unit(-3,1)
App.Units.DissipationRate             = App.Units.Unit(2,0,-3)
App.Units.DynamicViscosity            = App.Units.Unit(-1,1,-1)
App.Units.Frequency                   = App.Units.Unit(0,0,-1)
App.Units.MagneticFluxDensity         = App.Units.Unit(0,1,-2,-1)
App.Units.Magnetization               = App.Units.Unit(-1,0,0,1)
App.Units.ElectricalCapacitance       = App.Units.Unit(-2,-1,4,2)
App.Units.ElectricalConductance       = App.Units.Unit(-2,-1,3,2)
App.Units.ElectricalConductivity      = App.Units.Unit(-3,-1,3,2)
App.Units.ElectricalInductance        = App.Units.Unit(2,1,-2,-2)
App.Units.ElectricalResistance        = App.Units.Unit(2,1,-3,-2)
App.Units.ElectricCharge              = App.Units.Unit(0,0,1,1)
App.Units.ElectricPotential           = App.Units.Unit(2,1,-3,-1)
App.Units.Force                       = App.Units.Unit(1,1,-2)
App.Units.HeatFlux                    = App.Units.Unit(0,1,-3,0,0)
App.Units.InverseArea                 = App.Units.Unit(-2)
App.Units.InverseLength               = App.Units.Unit(-1)
App.Units.InverseVolume               = App.Units.Unit(-3)
App.Units.KinematicViscosity          = App.Units.Unit(2,0,-1)
App.Units.Pressure                    = App.Units.Unit(-1,1,-2)
App.Units.Power                       = App.Units.Unit(2,1,-3)
App.Units.ShearModulus                = App.Units.Unit(-1,1,-2)
App.Units.SpecificEnergy              = App.Units.Unit(2,0,-2)
App.Units.SpecificHeat                = App.Units.Unit(2,0,-2,0,-1)
App.Units.Stiffness                   = App.Units.Unit(0,1,-2)
App.Units.Stress                      = App.Units.Unit(-1,1,-2)
App.Units.ThermalConductivity         = App.Units.Unit(1,1,-3,0,-1)
App.Units.ThermalExpansionCoefficient = App.Units.Unit(0,0,0,0,-1)
App.Units.ThermalTransferCoefficient  = App.Units.Unit(0,1,-3,0,-1)
App.Units.UltimateTensileStrength     = App.Units.Unit(-1,1,-2)
App.Units.Velocity                    = App.Units.Unit(1,0,-1)
App.Units.VacuumPermittivity          = App.Units.Unit(-3,-1,4,2)
App.Units.Volume                      = App.Units.Unit(3)
App.Units.VolumeFlowRate              = App.Units.Unit(3,0,-1)
App.Units.VolumetricThermalExpansionCoefficient = App.Units.Unit(0,0,0,0,-1)
App.Units.Work                        = App.Units.Unit(2,1,-2)
App.Units.YieldStrength               = App.Units.Unit(-1,1,-2)
App.Units.YoungsModulus               = App.Units.Unit(-1,1,-2)


# Add an enum for the different unit schemes
from enum import IntEnum

# The values must match with that of the
# C++ enum class UnitSystem
class Scheme(IntEnum):
    SI1 = 0
    SI2 = 1
    Imperial1 = 2
    ImperialDecimal = 3
    Centimeters = 4
    ImperialBuilding = 5
    MmMin = 6
    ImperialCivil = 7
    FemMilliMeterNewton = 8

App.Units.Scheme = Scheme

class NumberFormat(IntEnum):
    Default = 0
    Fixed = 1
    Scientific = 2

App.Units.NumberFormat = NumberFormat

class ScaleType(IntEnum):
    Other = -1
    NoScaling = 0
    NonUniformRight = 1
    NonUniformLeft = 2
    Uniform = 3

App.ScaleType = ScaleType

class PropertyType(IntEnum):
    Prop_None = 0
    Prop_ReadOnly = 1
    Prop_Transient = 2
    Prop_Hidden = 4
    Prop_Output = 8
    Prop_NoRecompute = 16
    Prop_NoPersist = 32

App.PropertyType = PropertyType

class ReturnType(IntEnum):
    PyObject = 0
    DocObject = 1
    DocAndPyObject = 2
    Placement = 3
    Matrix = 4
    LinkAndPlacement = 5
    LinkAndMatrix = 6

App.ReturnType = ReturnType

# clean up namespace
del(InitApplications)

Log ('Init: App::FreeCADInit.py done\n')
