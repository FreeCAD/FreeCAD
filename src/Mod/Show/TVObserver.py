# /***************************************************************************
# *   Copyright (c) 2019 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

import FreeCAD


class TVObserver(object):
    def __init__(self):
        FreeCAD.addDocumentObserver(self)

    def stop(self):
        FreeCAD.removeDocumentObserver(self)

    def slotStartSaveDocument(self, doc, filepath):
        from . import TVStack

        TVStack._slotStartSaveDocument(doc)

    def slotFinishSaveDocument(self, doc, filepath):
        from . import TVStack

        TVStack._slotFinishSaveDocument(doc)

    def slotDeletedDocument(self, doc):
        from . import TVStack

        TVStack._slotDeletedDocument(doc)


# handle module reload
if "observer_singleton" in vars():
    observer_singleton.stop()

observer_singleton = TVObserver()
