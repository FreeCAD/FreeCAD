# /***************************************************************************
# *   Copyright (c) 2018 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

# This is a temporary replacement for C++-powered Container class that should be eventually introduced into FreeCAD


class Container(object):
    """Container class: a unified interface for container objects, such as Group, Part, Body, or Document.
    This is a temporary implementation."""

    Object = None  # DocumentObject or Document, the actual container

    def __init__(self, obj):
        self.Object = obj

    def self_check(self):
        if self.Object is None:
            raise ValueError("Null!")
        if not isAContainer(self.Object, links_too=True):
            raise NotAContainerError(self.Object)

    def getAllChildren(self):
        """Returns all objects directly contained by the container. all = static + dynamic."""
        return self.getStaticChildren() + self.getDynamicChildren()

    def getStaticChildren(self):
        """Returns children tightly bound to the container, such as Origin. The key thing
        about them is that they are not supposed to be removed or added from/to the container."""

        self.self_check()
        container = self.Object
        if container.isDerivedFrom("App::Document"):
            return []
        elif container.hasExtension("App::OriginGroupExtension"):
            if container.Origin is not None:
                return [container.Origin]
            else:
                return []
        elif container.isDerivedFrom("App::Origin"):
            return container.OriginFeatures
        elif container.hasExtension("App::GroupExtension"):
            return []
        elif container.hasChildElement():  # Link
            return []
        raise RuntimeError("getStaticChildren: unexpected container type!")

    def getDynamicChildren(self):
        """Returns dynamic children, i.e. the stuff that can be removed from the container."""
        self.self_check()
        container = self.Object

        if container.isDerivedFrom("App::Document"):
            # find all objects not contained by any Part or Body
            result = set(container.Objects)
            for obj in container.Objects:
                if isAContainer(obj):
                    children = set(Container(obj).getAllChildren())
                    result = result - children
            return list(result)
        elif container.hasExtension("App::GroupExtension"):
            result = container.Group
            if container.hasExtension("App::GeoFeatureGroupExtension"):
                # geofeaturegroup's group contains all objects within the CS, we don't want that
                result = [obj for obj in result if obj.getParentGroup() is not container]
            return result
        elif container.isDerivedFrom("App::Origin"):
            return []
        elif container.hasChildElement():
            result = []
            for sub in container.getSubObjects(1):
                sobj = container.getSubObject(sub, retType=1)
                if sobj:
                    result.append(sobj)
            return result

        raise RuntimeError("getDynamicChildren: unexpected container type!")

    def isACS(self):
        """isACS(): returns true if the container forms internal coordinate system."""
        self.self_check()
        container = self.Object

        if container.isDerivedFrom("App::Document"):
            return True  # Document is a special thing... is it a CS or not is a matter of coding convenience.
        elif container.hasExtension("App::GeoFeatureGroupExtension"):
            return True
        elif container.hasChildElement():  # Link
            return True
        else:
            return False

    def isAVisGroup(self):
        """isAVisGroup(): returns True if the container consumes viewproviders of children, and thus affects their visibility."""
        self.self_check()
        container = self.Object

        if container.isDerivedFrom("App::Document"):
            return True  # Document is a special thing... Return value is a matter of coding convenience.
        elif container.hasExtension("App::GeoFeatureGroupExtension"):
            return True
        elif container.isDerivedFrom("App::Origin"):
            return True
        elif container.hasChildElement():  # Link
            return True
        else:
            return False

    def getCSChildren(self):
        if not self.isACS():
            raise TypeError("Container is not a coordinate system")
        container = self.Object
        return _getMetacontainerChildren(self, Container.isACS)

    def getVisGroupChildren(self):
        if not self.isAVisGroup():
            raise TypeError("Container is not a visibility group")
        container = self.Object
        return _getMetacontainerChildren(self, Container.isAVisGroup)

    def isChildVisible(self, obj):
        container = self.Object
        isElementVisible = getattr(container, "isElementVisible", None)
        if not isElementVisible:
            return obj.Visibility
        vis = isElementVisible(obj.Name)
        if vis < 0:
            return obj.Visibility
        return vis > 0

    def hasObject(self, obj):
        """Returns True if the container contains specified object directly."""
        return obj in self.getAllChildren()

    def hasObjectRecursive(self, obj):
        return self.Object in ContainerChain(obj)


def _getMetacontainerChildren(container, isrightcontainer_func):
    """Gathers up children of metacontainer - a container structure formed by containers of specific type.
    For example, coordinate systems form a kind of container structure.

    container: instance of Container class
    isrightcontainer_func: a function f(cnt)->bool, where cnt is a Container object."""

    result = []
    list_traversing_now = [container]  # list of Container instances
    list_to_be_traversed_next = []  # list of Container instances
    visited_containers = set([container.Object])  # set of DocumentObjects

    while len(list_traversing_now) > 0:
        list_to_be_traversed_next = []
        for itcnt in list_traversing_now:
            children = itcnt.getAllChildren()
            result.extend(children)
            for child in children:
                if isAContainer(child):
                    newcnt = Container(child)
                    if not isrightcontainer_func(newcnt):
                        list_to_be_traversed_next.append(newcnt)
        list_traversing_now = list_to_be_traversed_next

    return result


def isAContainer(obj, links_too=False):
    """isAContainer(obj, links_too): returns True if obj is an object container, such as
    Group, Part, Body. The important characteristic of an object being a
    container is that it can be activated to receive new objects. Documents
    are considered containers, too.
    If links_too, App::Link objects are considered containers, too. Then, container tree
    isn't necessarily a tree."""

    if obj.isDerivedFrom("App::Document"):
        return True
    if obj.hasExtension("App::GroupExtension"):
        return True
    if obj.isDerivedFrom("App::Origin"):
        return True
    if obj.hasChildElement():
        return True if links_too else False
    return False


# from Part-o-magic...
def ContainerOf(obj):
    """ContainerOf(obj): returns the container that immediately has obj."""
    cnt = None
    for dep in obj.InList:
        if isAContainer(dep):
            if Container(dep).hasObject(obj):
                if cnt is not None and dep is not cnt:
                    raise ContainerTreeError("Container tree is not a tree")
                cnt = dep
    if cnt is None:
        return obj.Document
    return cnt


def getVisGroupOf(obj):
    chain = VisGroupChain(obj)
    return chain[-1]


# from Part-o-magic... over-engineered, but proven to work
def ContainerChain(feat):
    """ContainerChain(feat): container path to feat (not including feat itself).
    Last container directly contains the feature.
    Example of output:  [<document>,<SuperPart>,<Part>,<Body>]"""

    if feat.isDerivedFrom("App::Document"):
        return []

    list_traversing_now = [feat]
    set_of_deps = set()
    list_of_deps = []

    while len(list_traversing_now) > 0:
        list_to_be_traversed_next = []
        for feat in list_traversing_now:
            for dep in feat.InList:
                if isAContainer(dep) and Container(dep).hasObject(feat):
                    if not (dep in set_of_deps):
                        set_of_deps.add(dep)
                        list_of_deps.append(dep)
                        list_to_be_traversed_next.append(dep)
        if len(list_to_be_traversed_next) > 1:
            raise ContainerTreeError("Container tree is not a tree")
        list_traversing_now = list_to_be_traversed_next

    return [feat.Document] + list_of_deps[::-1]


def CSChain(feat):
    cnt_chain = ContainerChain(feat)
    return [cnt for cnt in cnt_chain if Container(cnt).isACS()]


def VisGroupChain(feat):
    cnt_chain = ContainerChain(feat)
    return [cnt for cnt in cnt_chain if Container(cnt).isAVisGroup()]


class ContainerError(RuntimeError):
    pass


class NotAContainerError(ContainerError):
    def __init__(self, name="None"):
        ContainerError.__init__(self, "'{}' is not recognized as container".format(name))


class ContainerTreeError(ContainerError):
    pass
