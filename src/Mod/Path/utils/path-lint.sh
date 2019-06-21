#!/bin/bash
#
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

# Script to run pylint on Path. Currently only checks for errors.

if [ 'utils' == $(basename ${PWD}) ]; then
  cd ..
elif [ 'PathScripts' == $(basename ${PWD}) ]; then
  cd ..
elif [ 'PathTests' == $(basename ${PWD}) ]; then
  cd ..
elif [ -d 'src/Mod/Path' ]; then
  cd src/Mod/Path
elif [ -d 'Mod/Path' ]; then
  cd Mod/Path
elif [ -d 'Path' ]; then
  cd Path
fi

if [ ! -d 'PathScripts' ]; then
  echo "Cannot determine source directory, please call from within Path source directory."
  exit 2
fi

EXTERNAL_MODULES+=' ArchPanel'
EXTERNAL_MODULES+=' Draft'
EXTERNAL_MODULES+=' DraftGeomUtils'
EXTERNAL_MODULES+=' DraftVecUtils'
EXTERNAL_MODULES+=' FreeCAD'
EXTERNAL_MODULES+=' FreeCADGui'
EXTERNAL_MODULES+=' Mesh'
EXTERNAL_MODULES+=' MeshPart'
EXTERNAL_MODULES+=' Part'
EXTERNAL_MODULES+=' Path'
EXTERNAL_MODULES+=' PySide'
EXTERNAL_MODULES+=' PySide.QtCore'
EXTERNAL_MODULES+=' PySide.QtGui'
EXTERNAL_MODULES+=' TechDraw'
EXTERNAL_MODULES+=' area'
EXTERNAL_MODULES+=' importlib'
EXTERNAL_MODULES+=' pivy'

ARGS+=" --errors-only"
ARGS+=" --ignored-modules=$(echo ${EXTERNAL_MODULES} | tr ' ' ',')"
ARGS+=" --jobs=4"

if [ -z "$(which pylint)" ]; then
  echo "Cannot find pylint, please install and try again!"
  exit 1
fi

#pylint ${ARGS} PathScripts/ PathTests/
pylint ${ARGS} PathScripts/
