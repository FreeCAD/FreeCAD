#!/bin/env python3
import hashlib
import os
import platform
import re
import shutil
import stat
import time
import subprocess
from shutil import rmtree


def run_cmd(cmd: list, return_output: bool = None):
    output_content = []
    if return_output is None:
        return_output = False

    def output(inp: subprocess.CalledProcessError | subprocess.Popen[bytes]):
        for o in iter(inp.stdout.readline, b""):
            try:
                out_put = o.decode("utf-8").strip()
            except (Exception, BaseException):
                out_put = o.decode("gbk").strip()
            if not return_output:
                print(out_put)
            else:
                output_content.append(out_put)

    conf = subprocess.CREATE_NO_WINDOW if os.name != 'posix' else 0
    try:
        ret = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, creationflags=conf)
        output(ret)
        if not return_output:
            return ret.returncode
        else:
            return output_content
    except subprocess.CalledProcessError as e:
        output(e)
        return 1


gpg_key = "yorik@freecad.org"
# the FreeCAD version we're looking for
target_version = "1.1.0"
# make sure target_python matches the one FreeCAD is built with! Check with
# conda search "freecad=1.1.0" -c adrianinsaval/label/dev -c freecad -c conda-forge
target_python = "3.10"
if not os.path.exists('AppDir'):
    os.makedirs('AppDir', exist_ok=True)
AppRunContent = """
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\${0}")")"
export PREFIX=\${HERE}/usr
export LD_LIBRARY_PATH=\${HERE}/usr/lib\${LD_LIBRARY_PATH:+':'}\$LD_LIBRARY_PATH
export PYTHONHOME=\${HERE}/usr
export PATH_TO_FREECAD_LIBDIR=\${HERE}/usr/lib
# export QT_QPA_PLATFORM_PLUGIN_PATH=\${HERE}/usr/plugins
# export QT_XKB_CONFIG_ROOT=\${HERE}/usr/lib
export FONTCONFIG_FILE=/etc/fonts/fonts.conf
export FONTCONFIG_PATH=/etc/fonts

# Show packages info if DEBUG env variable is set
if [ "\$DEBUG" = 1 ]; then
    cat \${HERE}/packages.txt
fi

# SSL
# https://forum.freecadweb.org/viewtopic.php?f=4&t=34873&start=20#p327416
export SSL_CERT_FILE=\$PREFIX/ssl/cacert.pem
# https://github.com/FreeCAD/FreeCAD-AppImage/pull/20
export GIT_SSL_CAINFO=\$HERE/usr/ssl/cacert.pem
# Support for launching other applications (from /usr/bin)
# https://github.com/FreeCAD/FreeCAD-AppImage/issues/30
if [ ! -z "\$1" ] && [ -e "\$HERE/usr/bin/\$1" ] ; then
    MAIN="\$HERE/usr/bin/\$1" ; shift
else
    MAIN="\$HERE/usr/bin/freecad"
fi

\${MAIN} "\$@"
"""
AppRunPath = "AppDir/AppRun"
with open(AppRunPath, "w") as f:
    f.write(AppRunContent)
st = os.stat(AppRunPath)
os.chmod(AppRunPath, st.st_mode | stat.S_IEXEC)  # chmod +x
target_date = time.strftime("%Y-%m-%d")
arch = platform.machine()
package_name = f"FreeCAD_{target_version}-{target_date}-conda-Linux-{arch}-py{target_python.replace('.', '')}"
conda_env = "AppDir/usr"
# dependencies
print("\nCreating the environment")
packages = [f'freecad={target_version}', 'occt', 'vtk', f'python={target_python}', 'blas=*=openblas', 'numpy',
            'matplotlib-base', 'scipy', 'sympy',
            'pandas', 'six', 'pyyaml', 'pycollada', 'lxml', 'xlutils', 'olefile', 'requests', 'blinker', 'opencv',
            'qt.py', 'nine', 'docutils',
            'calculix', 'opencamlib', 'ifcopenshell', 'appimage-updater-bridge']

run_cmd(
    ['mamba', 'create', '-p', conda_env, *packages, '--copy', '-c', 'adrianinsaval/label/dev', '-c', 'freecad', '-c',
     'conda-forge', '-y'])
print("\n################")
print(f"package_name:  {package_name}")
print("################")

print("\nInstalling freecad.appimage_updater")
run_cmd(['mamba', 'run', '-p', conda_env, 'pip', 'install',
         'https://github.com/looooo/freecad.appimage_updater/archive/master.zip'])
print("\nUninstalling some unneeded packages")
run_cmd(['conda', 'uninstall', '-p', conda_env, 'libclang', '--force', '-y'])

packages_content = run_cmd(["mamba", "list", "-p", conda_env])
packages_content[0] = '\nLIST OF PACKAGES:\n'
with open('AppDir/packages.txt', 'w') as f:
    f.writelines(packages_content)

print("\nDeleting unnecessary stuff")
rmtree(f"{conda_env}/include")
for root, _, files in os.walk(conda_env):
    for filename in files:
        file = os.path.join(root, filename)
        if file.endswith(".a"):
            os.remove(file)
shutil.move(f'{conda_env}/bin', f'{conda_env}/bin_tmp')
os.makedirs(f'{conda_env}/bin')
for i in ['freecad', 'freecadcmd', 'ccx', 'python', 'pip', 'pyside2-rcc', 'assistant']:
    shutil.copy(f'{conda_env}/bin_tmp/{i}', f'${conda_env}/bin/{i}')
with open(f'{conda_env}/bin/pip', 'r', encoding='utf-8') as f:
    pip_content = f.readlines()
    pip_content[0] = '#!/usr/bin/env python\n'
with open(f'{conda_env}/bin/pip', 'w', encoding='utf-8', newline='\n') as f:
    f.writelines(pip_content)
rmtree(f'{conda_env}/bin_tmp')
print("\nCreating qt config")
with open(f'{conda_env}/bin/qt.conf', 'w', encoding='utf-8', newline='\n') as f:
    f.write("[Paths]\nPrefix = ./../")
shutil.copy(f'{conda_env}/bin/qt.conf', f'{conda_env}/bin/libexec/qt.conf')

print("\nCopying icons and .desktop file")
os.makedirs(f'{conda_env}/share/icons/hicolor/scalable/apps/', exist_ok=True)
shutil.copy("../../../src/Gui/Icons/freecad.svg",
            f"{conda_env}/share/icons/hicolor/scalable/apps/org.freecadweb.FreeCAD.svg")
shutil.copy(f"{conda_env}/share/icons/hicolor/scalable/apps/org.freecadweb.FreeCAD.svg", "AppDir/org.freecadweb.FreeCAD.svg")
os.makedirs(f'{conda_env}/share/icons/hicolor/64x64/apps/')
shutil.copy("../../../src/Gui/Icons/freecad-icon-64.png", f"{conda_env}/share/icons/hicolor/64x64/apps/org.freecadweb.FreeCAD.png")
shutil.copy(f'{conda_env}/share/icons/hicolor/64x64/apps/org.freecad.FreeCAD.png', "AppDir/org.freecad.FreeCAD.png")
os.makedirs(f'{conda_env}/share/applications/')
desktop_path = f'{conda_env}/share/applications/org.freecad.FreeCAD.desktop'
shutil.copy('../../../src/XDGData/org.freecad.FreeCAD.desktop',desktop_path)
with open(desktop_path, 'r', encoding='utf-8') as f:
    desktop_content = f.readlines()
with open(desktop_path, 'w', encoding='utf-8', newline='\n') as f:
    for i in desktop_content:
        if i.startswith('Exec='):
            i = re.sub('Exec=FreeCAD.*', 'Exec=AppRun', i)
        f.write(i)
shutil.copy(desktop_path,'AppDir/org.freecad.FreeCAD.desktop')
shutil.copy(f'../../../src/XDGData/org.freecad.FreeCAD.appdata.xml.in', f'{conda_env}/share/metainfo/org.freecad.FreeCAD.appdata.xml')
with open(f'{conda_env}/share/metainfo/org.freecad.FreeCAD.appdata.xml', 'r', encoding='utf-8') as f:
    appdata_content = f.read()
    appdata_content = appdata_content.replace('@PACKAGE_VERSION@', target_version)
    appdata_content = appdata_content.replace('@APPDATA_RELEASE_DATE@', target_date)

with open(f'{conda_env}/share/metainfo/org.freecad.FreeCAD.appdata.xml', 'w', encoding='utf-8', newline='\n') as f:
    f.write(appdata_content)

print("\nCleaning")
for root, folders, files in os.walk(conda_env):
    for folder_name in folders:
        folder = os.path.join(root, folder_name)
        if folder_name == '__pycache__': # Remove __pycache__ folders and .pyc files
            rmtree(folder)
    for filename in files:
        file = os.path.join(root, filename)
        if file.endswith(".pyc"):
            os.remove(file)
        # remove unnecessary development files
        if file.endswith(".h"):
            os.remove(file)
        if file.endswith(".cmake"):
            os.remove(file)

for i in [f"{conda_env}/conda-meta/", f"{conda_env}/doc/global/", f"{conda_env}/share/gtk-doc/", f'{conda_env}/lib/cmake/']:
    rmtree(i)

# The following two lines must be uncommented if using this on Fedora 28 and up
# echo "\nAdd libnsl"
# cp ../../libc6/lib/$ARCH-linux-gnu/libnsl* ${conda_env}/lib/
print("\nCreating the appimage")
st = os.stat(AppRunPath)
os.chmod(AppRunPath, 33277)
run_cmd([f"appimagetool-{arch}.AppImage", "--sign", "--sign-key", gpg_key, "AppDir", f"{package_name}.AppImage"])
print("\nCreating hash")
def file_sha256(file_path: str) -> str:
    if not os.path.isfile(file_path):
        print('File not Exist.')
        return ''
    h = hashlib.sha256()
    with open(file_path, 'rb') as s:
        while b := s.read(8192):
            h.update(b)
    return h.hexdigest()
with open(f'{package_name}.AppImage-SHA256.txt','w', encoding='utf-8', newline='\n') as f:
    f.write(file_sha256(f"{package_name}.AppImage"))
print("\nAll done! You can delete the AppDir folder")
