# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""This module provides the object code for the Draft Link object.
"""
## @package draftlink
# \ingroup DRAFT
# \brief This module provides the object code for the Draft Link object.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

from draftutils.utils import get_param

from draftobjects.base import DraftObject


class DraftLink(DraftObject):
    """
    Documentation needed.
    DraftLink was introduced by Realthunder to allow the use of new 
    App::Link object into Draft Array objects during version 0.19 development
    cycle.
    """

    def __init__(self,obj,tp):
        self.use_link = False if obj else True
        super(DraftLink, self).__init__(obj, tp)
        if obj:
            self.attach(obj)

    def __getstate__(self):
        return self.__dict__

    def __setstate__(self,state):
        if isinstance(state,dict):
            self.__dict__ = state
        else:
            self.use_link = False
            super(DraftLink, self).__setstate__(state)

    def attach(self,obj):
        if self.use_link:
            obj.addExtension('App::LinkExtensionPython', None)
            self.linkSetup(obj)

    def canLinkProperties(self,_obj):
        return False

    def linkSetup(self,obj):
        obj.configLinkProperty('Placement',LinkedObject='Base')
        if hasattr(obj,'ShowElement'):
            # rename 'ShowElement' property to 'ExpandArray' to avoid conflict
            # with native App::Link
            obj.configLinkProperty('ShowElement')
            showElement = obj.ShowElement
            obj.addProperty("App::PropertyBool","ExpandArray","Draft",
                    QT_TRANSLATE_NOOP("App::Property","Show array element as children object"))
            obj.ExpandArray = showElement
            obj.configLinkProperty(ShowElement='ExpandArray')
            obj.removeProperty('ShowElement')
        else:
            obj.configLinkProperty(ShowElement='ExpandArray')
        if getattr(obj,'ExpandArray',False):
            obj.setPropertyStatus('PlacementList','Immutable')
        else:
            obj.setPropertyStatus('PlacementList','-Immutable')
        if not hasattr(obj,'LinkTransform'):
            obj.addProperty('App::PropertyBool','LinkTransform',' Link')
        if not hasattr(obj,'ColoredElements'):
            obj.addProperty('App::PropertyLinkSubHidden','ColoredElements',' Link')
            obj.setPropertyStatus('ColoredElements','Hidden')
        obj.configLinkProperty('LinkTransform','ColoredElements')

    def getViewProviderName(self,_obj):
        if self.use_link:
            return 'Gui::ViewProviderLinkPython'
        return ''

    def migrate_attributes(self, obj):
        """Migrate old attribute names to new names if they exist.

        This is done to comply with Python guidelines or fix small issues
        in older code.
        """
        if hasattr(self, "useLink"):
            # This is only needed for some models created in 0.19
            # while it was in development. Afterwards,
            # all models should use 'use_link' by default
            # and this won't be run.
            self.use_link = bool(self.useLink)
            App.Console.PrintWarning("Migrating 'useLink' to 'use_link', "
                                         "{} ({})\n".format(obj.Label,
                                                            obj.TypeId))
            del self.useLink

    def onDocumentRestored(self, obj):
        self.migrate_attributes(obj)
        if self.use_link:
            self.linkSetup(obj)
        else:
            obj.setPropertyStatus('Shape','-Transient')
        if obj.Shape.isNull():
            if getattr(obj,'PlacementList',None):
                self.buildShape(obj,obj.Placement,obj.PlacementList)
            else:
                self.execute(obj)

    def buildShape(self,obj,pl,pls):
        import Part
        import DraftGeomUtils

        if self.use_link:
            if not getattr(obj,'ExpandArray',True) or obj.Count != len(pls):
                obj.setPropertyStatus('PlacementList','-Immutable')
                obj.PlacementList = pls
                obj.setPropertyStatus('PlacementList','Immutable')
                obj.Count = len(pls)

        if obj.Base:
            shape = Part.getShape(obj.Base)
            if shape.isNull():
                raise RuntimeError("'{}' cannot build shape of '{}'\n".format(
                        obj.Name,obj.Base.Name))
            else:
                shape = shape.copy()
                shape.Placement = App.Placement()
                base = []
                for i,pla in enumerate(pls):
                    vis = getattr(obj,'VisibilityList',[])
                    if len(vis)>i and not vis[i]:
                        continue
                    # 'I' is a prefix for disambiguation when mapping element names
                    base.append(shape.transformed(pla.toMatrix(), op='I{}'.format(i)))
                if getattr(obj,'Fuse',False) and len(base) > 1:
                    obj.Shape = base[0].multiFuse(base[1:]).removeSplitter()
                else:
                    obj.Shape = Part.makeCompound(base)

                if not DraftGeomUtils.isNull(pl):
                    obj.Placement = pl

        if self.use_link:
            return False # return False to call LinkExtension::execute()

    def onChanged(self, obj, prop):
        if not getattr(self, 'use_link', False):
            return
        if prop == 'Fuse':
            if obj.Fuse:
                obj.setPropertyStatus('Shape', '-Transient')
            else:
                obj.setPropertyStatus('Shape', 'Transient')
        elif prop == 'ExpandArray':
            if hasattr(obj,'PlacementList'):
                obj.setPropertyStatus('PlacementList',
                        '-Immutable' if obj.ExpandArray else 'Immutable')


_DraftLink = DraftLink
