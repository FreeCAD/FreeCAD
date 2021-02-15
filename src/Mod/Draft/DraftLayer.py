# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
"""Provides the Layer object. This module is deprecated.

In 6f896d8f22 (April 2014) the `Layer` object was created,
but in 4e595bd7bb (June 2014) it was renamed to `VisGroup`.
However, it was not used a lot, so in commit 5ee99ca4e (June 2019)
it was renamed again to `Layer`, but this time it was improved to behave
more like a proper layer system to control the visual properties
of the contained objects. All new code was moved to this module.

With the reorganization of the entire Draft workbench, the Layer object
and associated viewprovider, make function, and Gui Command
have been moved to the appropriate directories `draftobjects`,
`draftviewproviders`, `draftmake`, and `draftguitools`.
Therefore, this module is only required to migrate old objects
created with v0.18 and earlier, and certain development version of v0.19.

Since this module is only used to migrate older objects, it is only temporary,
and will be removed after one year, that is, in July 2021.

The explanation of the migration methods is in the wiki page:
https://wiki.freecadweb.org/Scripted_objects_migration
"""
## @package DraftLayer
# \ingroup DRAFT
# \brief Provides the Layer object. This module is deprecated.
#
# This module is only required to migrate old objects created
# with v0.18 and earlier and with certain development version of v0.19.
# It will be removed definitely in January 2021.
import FreeCAD as App

from draftobjects.layer import (Layer,
                                _VisGroup,
                                LayerContainer)

if App.GuiUp:
    from draftviewproviders.view_layer import (ViewProviderLayer,
                                               _ViewProviderVisGroup,
                                               ViewProviderLayerContainer)
