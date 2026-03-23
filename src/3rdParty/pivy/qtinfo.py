import os, sys
import subprocess
from distutils.spawn import find_executable

class QtInfo(object):
    def __init__(self, qmake_command=None):
        if qmake_command:
            self._qmake_command = qmake_command
        else:
            self._qmake_command = [find_executable("qmake"),]
        self._dict = {}
        # bind all variables early at __init__ time.
        for thing in self.__class__.__dict__:
            getattr(self, thing)

    def getQMakeCommand(self):
        qmake_command_string = self._qmake_command[0]
        for entry in self._qmake_command[1:]:
            qmake_command_string += " %s" %(entry)
        return qmake_command_string

    def getVersion(self):
        return self.getProperty("QT_VERSION")

    def getBinsPath(self):
        return self.getProperty("QT_INSTALL_BINS")

    def getLibsPath(self):
        return self.getProperty("QT_INSTALL_LIBS")

    def getLibsExecsPath(self):
        return self.getProperty("QT_INSTALL_LIBEXECS")

    def getPluginsPath(self):
        return self.getProperty("QT_INSTALL_PLUGINS")

    def getPrefixPath(self):
        return self.getProperty("QT_INSTALL_PREFIX")

    def getImportsPath(self):
        return self.getProperty("QT_INSTALL_IMPORTS")

    def getTranslationsPath(self):
        return self.getProperty("QT_INSTALL_TRANSLATIONS")

    def getHeadersPath(self):
        return self.getProperty("QT_INSTALL_HEADERS")

    def getDocsPath(self):
        return self.getProperty("QT_INSTALL_DOCS")

    def getQmlPath(self):
        return self.getProperty("QT_INSTALL_QML")

    def _getProperty(self, prop_name):
        cmd = self._qmake_command + ["-query", prop_name]
        proc = subprocess.Popen(cmd, stdout = subprocess.PIPE, shell=False)
        prop = proc.communicate()[0]
        proc.wait()
        if proc.returncode != 0:
            return None
        if sys.version_info >= (3,):
            return str(prop, 'ascii').strip()
        return prop.strip()

    def getProperty(self, prop_name):
        if prop_name not in self._dict:
            self._dict[prop_name] = self._getProperty(prop_name)
        return self._dict[prop_name]

    version = property(getVersion)
    bins_dir = property(getBinsPath)
    libs_dir = property(getLibsPath)
    lib_execs_dir = property(getLibsExecsPath)
    plugins_dir = property(getPluginsPath)
    prefix_dir = property(getPrefixPath)
    qmake_command = property(getQMakeCommand)
    imports_dir = property(getImportsPath)
    translations_dir = property(getTranslationsPath)
    headers_dir = property(getHeadersPath)
    docs_dir = property(getDocsPath)
    qml_dir = property(getQmlPath)