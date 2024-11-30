# /***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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


def getAllDependencies(feat):
    """getAllDependencies(feat): gets all features feat depends on, directly or indirectly.
    Returns a list, with deepest dependencies last. feat is not included in the list, except
    if the feature depends on itself (dependency loop)."""
    list_traversing_now = [feat]
    set_of_deps = set()
    list_of_deps = []

    while len(list_traversing_now) > 0:
        list_to_be_traversed_next = []
        for feat in list_traversing_now:
            for dep in feat.OutList:
                if not (dep in set_of_deps):
                    set_of_deps.add(dep)
                    list_of_deps.append(dep)
                    list_to_be_traversed_next.append(dep)

        list_traversing_now = list_to_be_traversed_next

    return list_of_deps


def getAllDependent(feat):
    """getAllDependent(feat): gets all features that depend on feat, directly or indirectly.
    Returns a list, with deepest dependencies last. feat is not included in the list, except
    if the feature depends on itself (dependency loop)."""
    list_traversing_now = [feat]
    set_of_deps = set()
    list_of_deps = []

    while len(list_traversing_now) > 0:
        list_to_be_traversed_next = []
        for feat in list_traversing_now:
            for dep in feat.InList:
                if not (dep in set_of_deps):
                    set_of_deps.add(dep)
                    list_of_deps.append(dep)
                    list_to_be_traversed_next.append(dep)

        list_traversing_now = list_to_be_traversed_next

    return list_of_deps
