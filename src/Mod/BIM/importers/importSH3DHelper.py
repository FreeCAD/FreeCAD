# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Julien Masnada <rostskadat@gmail.com>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Helper functions that are used by SH3D importer."""

import itertools
import math
import os
import re
import traceback
import uuid
import xml.etree.ElementTree as ET
import zipfile

import numpy as np

import FreeCAD as App
import Arch
import BOPTools.SplitFeatures
import BOPTools.BOPFeatures
import Draft
import DraftGeomUtils
import DraftVecUtils
import Mesh
import MeshPart
import Part
import TechDraw

from draftutils.messages import _err, _log, _msg, _wrn
from draftutils.params import get_param_arch

if App.GuiUp:
    import FreeCADGui as Gui
    from draftutils.translate import translate
else:
    # \cond
    def translate(_, text):
        return text
    # \endcond

# Used to make section edges more visible (https://coolors.co/5bc0eb-fde74c-9bc53d-e55934-fa7921)
DEBUG_EDGES_COLORS = ["5bc0eb", "fde74c", "9bc53d", "e55934", "fa7921"]
DEBUG_POINT_COLORS = ["011627", "ff0022", "41ead4", "fdfffc", "b91372"]
RED = (255,0,0,1)
GREEN = (0,255,0,1)
BLUE = (0,0,255,1)
MAGENTA = (255,85,255,1)
MAGENTA_LIGHT = (255,85,127,1)
ORANGE = (255,85,0,1)

try:
    from Render import Camera, PointLight
    from Render.project import Project
    RENDER_IS_AVAILABLE = True
except :
    RENDER_IS_AVAILABLE = False

# SweetHome3D is in cm while FreeCAD is in mm
FACTOR = 10
TOLERANCE = 1
DEFAULT_WALL_WIDTH = 100
TWO_PI = 2* math.pi
DEFAULT_MATERIAL = App.Material(
    DiffuseColor=(1.00,0.00,0.00),
    AmbientColor=(0.33,0.33,0.33),
    SpecularColor=(0.53,0.53,0.53),
    EmissiveColor=(0.00,0.00,0.00),
    Shininess=(0.90),
    Transparency=(0.00)
    )

ORIGIN = App.Vector(0, 0, 0)
X_NORM = App.Vector(1, 0, 0)
Y_NORM = App.Vector(0, 1, 0)
Z_NORM = App.Vector(0, 0, 1)
NO_ROT = App.Rotation()

# The Windows lookup map. This is really brittle and a better system should
# be found. Arch.WindowPresets =  ["Fixed", "Open 1-pane", "Open 2-pane",
#       "Sash 2-pane", "Sliding 2-pane", "Simple door", "Glass door",
#       "Sliding 4-pane", "Awning"]
# unzip -p all-windows.sh3d Home.xml | \
#   grep 'catalogId=' | \
#   sed -e 's/.*catalogId=//;s/ name=.*/: ("Open 1-pane","Window"),/' | sort -u
# unzip -p all-doors.sh3d Home.xml | \
#   grep 'catalogId=' | \
#   sed -e 's/.*catalogId=//;s/ name=.*/: ("Simple door","Door")/' | sort -u
DOOR_MODELS = {
    'eTeks#doorFrame': ("Opening only", "Opening Element"),
    'eTeks#door': ("Simple door","Door"),
    'eTeks#frontDoor': ("Simple door","Door"),
    'eTeks#garageDoor': ("Simple door","Door"),
    'eTeks#openDoor': ("Simple door","Door"),
    'eTeks#roundDoorFrame': ("Opening only", "Opening Element"),
    'eTeks#roundedDoor': ("Simple door","Door"),
    'Kator Legaz#exterior-door-01': ("Simple door","Door"),
    'Kator Legaz#exterior-door-02': ("Simple door","Door"),
    'Kator Legaz#exterior-door-03': ("Glass door","Door"),
    'Kator Legaz#exterior-door-05': ("Simple door","Door"),
    'Kator Legaz#exterior-door-07': ("Glass door","Door"),
    'Kator Legaz#screen-door': ("Simple door","Door"),
    'Scopia#door': ("Simple door","Door"),
    'Scopia#double_door_2': ("Simple door","Door"),
    'Scopia#double_door': ("Glass door","Door"),
    'Scopia#double_door_with_little_part': ("Glass door","Door"),
    'Scopia#elevator-door': ("Simple door","Door"),
    'Scopia#garage-door2': ("Simple door","Door"),
    'Scopia#garage-door': ("Simple door","Door"),
    'Scopia#glassDoor2': ("Glass door","Door"),
    'Scopia#glass_door': ("Glass door","Door"),
    'Scopia#puerta': ("Simple door","Door"),
    "PeterSmolik#door1": ("Simple door","Door"),
    "PeterSmolik#doorGlassPanels": ("Simple door","Door"),
    "PeterSmolik#door1": ("Simple door","Door"),
    'Siath#emergencyGlassDoubleDoor': ("Simple door","Door"),
    'OlaKristianHoff#door_window_thick_double_2x4': ("Simple door","Door"),
    'Mchnz#craftsmanDoorClosed': ("Simple door","Door"),

    'eTeks#doubleFrenchWindow126x200': ("Open 1-pane","Window"),
    'eTeks#doubleHungWindow80x122': ("Open 1-pane","Window"),
    'eTeks#doubleOutwardOpeningWindow': ("Open 1-pane","Window"),
    'eTeks#doubleWindow126x123': ("Open 1-pane","Window"),
    'eTeks#doubleWindow126x163': ("Open 1-pane","Window"),
    'eTeks#fixedTriangleWindow85x85': ("Open 1-pane","Window"),
    'eTeks#fixedWindow85x123': ("Fixed","Window"),
    'eTeks#frenchWindow85x200': ("Open 1-pane","Window"),
    'eTeks#halfRoundWindow': ("Open 1-pane","Window"),
    'eTeks#roundWindow': ("Open 1-pane","Window"),
    'eTeks#sliderWindow126x200': ("Open 1-pane","Window"),
    'eTeks#window85x123': ("Open 1-pane","Window"),
    'eTeks#window85x163': ("Open 1-pane","Window"),
    'eTeks#serviceHatch': ("Fixed","Window"),
    'Kator Legaz#window-01': ("Open 1-pane","Window"),
    'Kator Legaz#window-08-02': ("Open 1-pane","Window"),
    'Kator Legaz#window-08': ("Open 1-pane","Window"),
    'Scopia#turn-window': ("Open 1-pane","Window"),
    'Scopia#window_2x1_medium_with_large_pane': ("Open 1-pane","Window"),
    'Scopia#window_2x1_with_sliders': ("Open 1-pane","Window"),
    'Scopia#window_2x3_arched': ("Open 1-pane","Window"),
    'Scopia#window_2x3': ("Open 1-pane","Window"),
    'Scopia#window_2x3_regular': ("Open 1-pane","Window"),
    'Scopia#window_2x4_arched': ("Open 1-pane","Window"),
    'Scopia#window_2x4': ("Open 1-pane","Window"),
    'Scopia#window_2x6': ("Open 1-pane","Window"),
    'Scopia#window_3x1': ("Open 1-pane","Window"),
    'Scopia#window_4x1': ("Open 1-pane","Window"),
    'Scopia#window_4x3_arched': ("Open 1-pane","Window"),
    'Scopia#window_4x3': ("Open 1-pane","Window"),
    'Scopia#window_4x5': ("Open 1-pane","Window"),
    'Artist373#rectangularFivePanesWindow': ("Open 1-pane","Window"),
    'OlaKristianHoff#window_shop': ("Open 1-pane","Window"),
    'OlaKristianHoff#window_double_2x3_frame_sill': ("Open 1-pane","Window"),
    'OlaKristianHoff#window_deep': ("Open 1-pane","Window"),
    'OlaKristianHoff#fixed_window_2x2': ("Fixed","Window"),
    'OlaKristianHoff#window_double_3x3': ("Open 1-pane","Window"),
}

ET_XPATH_LEVEL = 'level'
ET_XPATH_ROOM = 'room'
ET_XPATH_WALL = 'wall'
ET_XPATH_DOOR_OR_WINDOWS = './/doorOrWindow'
ET_XPATH_PIECE_OF_FURNITURE = './/pieceOfFurniture'
ET_XPATH_LIGHT = 'light'
ET_XPATH_OBSERVER_CAMERA = 'observerCamera'
ET_XPATH_CAMERA = 'camera'
ET_XPATH_DUMMY_SLAB = 'DummySlab'
ET_XPATH_DUMMY_DECORATE = 'DummyDecorate'

class Transaction(object):

    def __init__(self, title, doc= None):
        if doc is None:
            doc = App.ActiveDocument
        self.title = title
        self.document = doc

    def __enter__(self):
        self.document.openTransaction(self.title)

    def __exit__(self, exc_type, exc_value, exc_traceback):
        if exc_value is None:
            self.document.commitTransaction()
        elif DEBUG_GEOMETRY:
            _err(f"Transaction failed but DEBUG_GEOMETRY is set. Committing transaction anyway.")
            self.document.commitTransaction()
        else:
            self.document.abortTransaction()


class SH3DImporter:
    """The main class to import an SH3D file.

    As an implementation detail, note that we do not use an
    xml.sax parser as the XML elements found in the SH3D file
    do not follow a natural / dependency order (i.e. doors and
    windows depend upon wall but are usually defined *before*
    the different <wall> elements)
    """

    def __init__(self, progress_bar=None):
        """Create a SH3DImporter instance to import the given SH3D file.

        Args:
            progress_bar (ProgressIndicator,optional): a ProgressIndicator
              called to let the User monitor the import process
        """
        super().__init__()
        self.filename = None
        self.progress_bar = progress_bar
        self.preferences = {}
        self.handlers = {}
        self.total_object_count = 0
        self.current_object_count = 0
        self.zip = None
        self.fc_objects = {}
        self.project = None
        self.site = None
        self.building = None
        self.default_floor = None
        self.floors = {}
        self.walls = {}
        self.spaces = {}

    def import_sh3d_from_string(self, home:str):
        """Import the SH3D Home from a String.

        Args:
            home (str): the string containing the XML of the home
                to be imported.

        Raises:
            ValueError: if an invalid SH3D file is detected
        """
        self._get_preferences()
        self._setup_handlers()

        if self.progress_bar:
            self.progress_bar.start(f"Importing SweetHome 3D Home. Standby…", -1)
        self._import_home(ET.fromstring(home))

    def import_sh3d_from_filename(self, filename:str):
        """Import the SH3D file.

        Args:
            filename (str): the filename of the SH3D file to be imported.

        Raises:
            ValueError: if an invalid SH3D file is detected
        """
        self.filename = filename
        if App.GuiUp and get_param_arch("sh3dShowDialog"):
            Gui.showPreferences("Import-Export", 7)

        self._get_preferences()
        self._setup_handlers()

        if self.progress_bar:
            self.progress_bar.start(f"Importing SweetHome 3D file '{self.filename}'. Standby…", -1)
        with zipfile.ZipFile(self.filename, 'r') as zip:
            self.zip = zip
            entries = zip.namelist()
            if "Home.xml" not in entries:
                raise ValueError(f"Invalid SweetHome3D file {self.filename}: missing Home.xml")
            self._import_home(ET.fromstring(zip.read("Home.xml")))

    def _import_home(self, home):
        doc = App.ActiveDocument
        self.total_object_count = self._get_object_count(home)
        _msg(f"Importing home '{home.get('name')}'…")
        # Create the groups to organize the different resources together
        self._create_groups()

        # Get all the FreeCAD object in the active doc, in order to allow
        # for merge of existing object
        if self.preferences["MERGE"]:
            list(map(lambda o2: self.add_fc_objects(o2), list(filter(lambda o1: hasattr(o1, 'id'), doc.Objects))))

        # Let's create the project and site for this import
        self._setup_project(home)

        # Import the <level> element if any. If none are defined
        # create a default one.
        if home.find(ET_XPATH_LEVEL) != None:
            self._import_elements(home, ET_XPATH_LEVEL)
        else:
            # Has the default floor already been created from a
            # previous import?
            if self.preferences["DEBUG_GEOMETRY"]: _log("No level defined. Using default level…")
            self.default_floor = self.fc_objects.get('Level') if 'Level' in self.fc_objects else self._create_default_floor()

        # Importing <room> elements ...
        self._import_elements(home, ET_XPATH_ROOM)

        # Importing <wall> elements ...
        self._import_elements(home, ET_XPATH_WALL)
        self._refresh()

        # Walls&Rooms have been imported. Created the floor slabs
        self._create_slabs()
        self._refresh()

        if self.preferences["CREATE_GROUND_MESH"]:
            self._create_ground_mesh(home)
            self._refresh()

        if App.GuiUp and self.preferences["FIT_VIEW"]:
            Gui.SendMsgToActiveView("ViewFit")

        # Importing <doorOrWindow> elements ...
        if self.preferences["IMPORT_DOORS_AND_WINDOWS"]:
            self._import_elements(home, ET_XPATH_DOOR_OR_WINDOWS)
            self._refresh()

        # doorOrWndows have been imported. Now we can decorate...
        if self.preferences["DECORATE_SURFACES"]:
            self._decorate_surfaces()
            self._refresh()

        # Importing <pieceOfFurniture> && <furnitureGroup> elements ...
        if self.preferences["IMPORT_FURNITURES"]:
            self._import_elements(home, ET_XPATH_PIECE_OF_FURNITURE)
            self._refresh()

        # Importing <light> elements ...
        if self.preferences["IMPORT_LIGHTS"] or self.preferences["IMPORT_FURNITURES"]:
            self._import_elements(home, ET_XPATH_LIGHT)
            self._refresh()

        # Importing <observerCamera> elements ...
        if self.preferences["IMPORT_CAMERAS"]:
            self._import_elements(home, ET_XPATH_OBSERVER_CAMERA)
            self._import_elements(home, ET_XPATH_CAMERA)
            self._refresh()

        if self.preferences["CREATE_RENDER_PROJECT"] and self.site:
            Project.create(doc, renderer="Povray", template="povray_standard.pov")
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(self.site)
            Gui.runCommand('Render_View', 0)
            self._refresh()

        _msg(f"Successfully imported home '{home.get('name')}' …")

    def _get_object_count(self, home):
        """Get an approximate count of object to be imported
        """
        count = 0
        for tag in self.handlers.keys():
            count = count + len(list(home.findall(tag)))
        return count

    def _get_preferences(self):
        """Retrieve the SH3D preferences available in Mod/Arch."""
        self.preferences = {
            'IMPORT_DOORS_AND_WINDOWS': get_param_arch("sh3dImportDoorsAndWindows"),
            'IMPORT_FURNITURES': get_param_arch("sh3dImportFurnitures"),
            'IMPORT_LIGHTS': get_param_arch("sh3dImportLights") and RENDER_IS_AVAILABLE,
            'IMPORT_CAMERAS': get_param_arch("sh3dImportCameras") and RENDER_IS_AVAILABLE,
            'MERGE': get_param_arch("sh3dMerge"),
            'CREATE_ARCH_EQUIPMENT': get_param_arch("sh3dCreateArchEquipment"),
            'JOIN_ARCH_WALL': get_param_arch("sh3dJoinArchWall"),
            'CREATE_RENDER_PROJECT': get_param_arch("sh3dCreateRenderProject") and RENDER_IS_AVAILABLE,
            'FIT_VIEW': get_param_arch("sh3dFitView"),
            'CREATE_IFC_PROJECT': get_param_arch("sh3dCreateIFCProject"),
            'DEFAULT_FLOOR_COLOR': color_fc2sh(get_param_arch("sh3dDefaultFloorColor")),
            'DEFAULT_CEILING_COLOR': color_fc2sh(get_param_arch("sh3dDefaultCeilingColor")),
            'CREATE_GROUND_MESH': get_param_arch("sh3dCreateGroundMesh"),
            'DEFAULT_GROUND_COLOR': color_fc2sh(get_param_arch("sh3dDefaultGroundColor")),
            'DEFAULT_SKY_COLOR': color_fc2sh(get_param_arch("sh3dDefaultSkyColor")),
            'DECORATE_SURFACES': get_param_arch("sh3dDecorateSurfaces"),
            'DEFAULT_FURNITURE_COLOR': color_fc2sh(get_param_arch("sh3dDefaultFurnitureColor")),
            'DEBUG_GEOMETRY': get_param_arch("sh3dDebugGeometry"),
        }

    def _setup_handlers(self):
        self.handlers = {
            ET_XPATH_LEVEL: LevelHandler(self),
            ET_XPATH_ROOM: RoomHandler(self),
            ET_XPATH_WALL: WallHandler(self),
            ET_XPATH_DUMMY_SLAB : None,
        }
        if self.preferences["IMPORT_DOORS_AND_WINDOWS"]:
            self.handlers[ET_XPATH_DOOR_OR_WINDOWS] = DoorOrWindowHandler(self)

        if self.preferences["DECORATE_SURFACES"]:
            self.handlers[ET_XPATH_DUMMY_DECORATE] = None,

        if self.preferences["IMPORT_FURNITURES"]:
            self.handlers[ET_XPATH_PIECE_OF_FURNITURE] = FurnitureHandler(self)
            self.handlers[ET_XPATH_LIGHT] = LightHandler(self)

        if self.preferences["IMPORT_LIGHTS"]:
            self.handlers[ET_XPATH_LIGHT] = LightHandler(self)

        if self.preferences["IMPORT_CAMERAS"]:
            camera_handler = CameraHandler(self)
            self.handlers[ET_XPATH_OBSERVER_CAMERA] = camera_handler
            self.handlers[ET_XPATH_CAMERA] = camera_handler

    def _refresh(self):
        if App.GuiUp:
            Gui.updateGui()

    def set_property(self, obj, type_, name, description, value, valid_values=None, group="SweetHome3D"):
        """Set the attribute of the given object as an FC property

        Note that the method has a default behavior when the value is not specified.

        Args:
            obj (object): The FC object to add a property to
            type_ (str): the type of property to add
            name (str): the name of the property to add
            description (str): a short description of the property to add
            value (xml.etree.ElementTree.Element|str): The property's value. Defaults to None.
            valid_values (list): an optional list of valid values
        """

        self._add_property(obj, type_, name, description, group)
        if valid_values:
            setattr(obj, name, valid_values)
        if value is None:
            if self.preferences["DEBUG_GEOMETRY"]:_log(f"Setting obj.{name}=None")
            return
        if type(value) is ET.Element or type(value) is type(dict()):
            if type_ == "App::PropertyString":
                value = str(value.get(name, ""))
            elif type_ == "App::PropertyFloat":
                value = float(value.get(name, 0))
            elif type_ == "App::PropertyQuantity":
                value = float(value.get(name, 0))
            elif type_ == "App::PropertyInteger":
                value = int(value.get(name, 0))
            elif type_ == "App::PropertyPercent":
                value = int(value.get(name, 0))
            elif type_ == "App::PropertyBool":
                value = value.get(name, "true") == "true"
        if self.preferences["DEBUG_GEOMETRY"]:
            _log(f"Setting @{obj}.{name} = {value}")
        setattr(obj, name, value)

    def _add_property(self, obj, property_type, name, description, group="SweetHome3D"):
        """Add an property to the FC object.

        All properties will be added under the 'SweetHome3D' group

        Args:
            obj (object): TheFC object to add a property to
            property_type (str): the type of property to add
            name (str): the name of the property to add
            description (str): a short description of the property to add
        """
        if name not in obj.PropertiesList:
            obj.addProperty(property_type, name, group, description, locked=True)

    def add_fc_objects(self, obj):
        """Register `obj`.

        This object can then be referenced later on by
        other objects (i.e. light, etc.)

        Args:
            obj (AppDocumentObject): the object to register
        """
        self.fc_objects[obj.id] = obj

    def get_fc_object(self, id, sh_type):
        """Returns the FC doc element corresponding to the imported id and sh_type

        Args:
            id (str): the id of the element to lookup
            sh_type (str, optional): The SweetHome type of the element to be imported. Defaults to None.

        Returns:
            FCObject: The FC object that correspond to the imported SH element
        """
        if self.preferences["MERGE"] and id in self.fc_objects:
            fc_object = self.fc_objects[id]
            if sh_type:
                assert fc_object.shType == sh_type, f"Invalid shType: expected {sh_type}, got {fc_object.shType}"
            if self.preferences["DEBUG_GEOMETRY"]:
                _log(translate("BIM", f"Merging imported element '{id}' with existing element of type '{type(fc_object)}'"))
            return fc_object
        if self.preferences["DEBUG_GEOMETRY"]:
            _log(translate("BIM", f"No element found with id '{id}' and type '{sh_type}'"))
        return None

    def add_floor(self, floor):
        self.floors[floor.id] = floor
        self.building.addObject(floor)

    def get_all_floors(self):
        return self.floors.values()

    def get_floor(self, level_id):
        """Returns the Floor associated with the level_id.

        Returns the first level if only one defined or level_id is None

        Args:
            levels (list): The list of imported levels
            level_id (string): the level @id

        Returns:
            level: The level
        """
        if self.default_floor or not level_id:
            return self.default_floor
        return self.floors.get(level_id, None)

    def add_space(self, floor, space):
        if floor.id not in self.spaces:
            self.spaces[floor.id] = []
        self.spaces[floor.id].append(space)

    def get_all_spaces(self):
        return list(itertools.chain(*self.spaces.values()))

    def get_spaces(self, floor):
        return self.spaces.get(floor.id, [])

    def get_space(self, floor, p):
        """Returns the Space this point belongs to.

        An point belongs to a space if it is the closest space below that point

        Args:
            floor (level): the floor the point's parent belongs to.
            p (Point): the point for which to determine the closest space.

        Returns:
            Space: the space the object belongs to or None
        """
        closest_space = None
        for space in self.spaces.get(floor.id, []):
            space_face = space.Base.Shape
            space_z = space_face.CenterOfGravity.z
            projection = App.Vector(p.x, p.y, space_z)
            # Checks that:
            #   - the point's projection is inside the face
            #   - the point is above the face
            #   - the point's parent and the face's are on the same level
            # NOTE: If two rooms overlap on the same level, the result is
            #   undefined...
            if space_face.isInside(projection, 1, True) and space_z < p.z:
                closest_space = space
        return closest_space

    def add_wall(self, floor, wall):
        if floor.id not in self.walls:
            self.walls[floor.id] = []
        self.walls[floor.id].append(wall)

    def get_all_walls(self):
        """Returns a map of all the walls in the building grouped by floor

        Returns:
            dict: the map of all the walls
        """
        return list(itertools.chain(*self.walls.values()))

    def get_walls(self, floor):
      """Returns the wall belonging to the specified level

      Args:
          floor (Arch.Level): the level for which to return the list of wall

      Returns:
          list: the list of Arch.Wall
      """
      return self.walls.get(floor.id, [])

    def _create_groups(self):
        """Create FreeCAD Group for the different imported elements
        """
        doc = App.ActiveDocument
        if self.preferences["IMPORT_CAMERAS"] and not doc.getObject("Cameras"):
            _log(f"Creating Cameras group…")
            doc.addObject("App::DocumentObjectGroup", "Cameras")
        if self.preferences["DEBUG_GEOMETRY"] and not doc.getObject("DEBUG_GEOMETRY"):
            _log(f"Creating DEBUG_GEOMETRY group…")
            doc.addObject("App::DocumentObjectGroup", "DEBUG_GEOMETRY")

    def _setup_project(self, elm):
        """Create the Arch::Project and Arch::Site for this import

        Args:
            elm (str): the <home> element

        """
        if 'Site' in self.fc_objects:
            self.site = self.fc_objects.get('Site')
        else:
            self.site = self._create_site()
        self._set_site_properties(elm)

        if elm.get('name') in self.fc_objects:
            self.building = self.fc_objects.get(elm.get('name'))
        else:
            self.building = self._create_building(elm)

        self.site.addObject(self.building)

        if 'Project' in self.fc_objects:
            self.project = self.fc_objects.get('Project')
        elif self.preferences["CREATE_IFC_PROJECT"]:
            self.project = self._create_project()
        if self.project:
            self.project.addObject(self.site)

    def _create_project(self):
        """Create a default Arch::Project object
        """
        project = Arch.makeProject([])
        self.set_property(project, "App::PropertyString", "id", "The element's id", "Project")
        return project

    def _create_site(self):
        """Create a default Arch::Site object
        """
        site = Arch.makeSite([])
        self.set_property(site, "App::PropertyString", "id", "The element's id", "Site")
        return site

    def _create_building(self, elm):
        """Create a default Arch::Building object

        Args:
            elm (str): the <home> element

        Returns:
            the Arch::Building
        """
        building = Arch.makeBuilding([])
        self.set_property(building, "App::PropertyString", "shType", "The element type", 'building')
        self.set_property(building, "App::PropertyString", "id", "The element's id", elm.get('name'))
        for property in elm.findall('property'):
            name = re.sub('[^A-Za-z0-9]+', '', property.get('name'))
            value = property.get('value')
            self.set_property(building, "App::PropertyString", name, "", value)
        return building

    def _create_default_floor(self):
        """Create a default Arch::Floor object
        """
        return self.handlers['level'].create_default_floor()

    def _create_ground_mesh(self, elm):
        self.building.recompute(True)

        ground = None
        if self.preferences["MERGE"]:
            ground = self.get_fc_object('ground', 'ground')

        if not ground:
            bb = self.building.Shape.BoundBox
            dx = bb.XLength/2
            dy = bb.YLength/2
            SO = App.Vector(bb.XMin-dx, bb.YMin-dy, 0)
            NO = App.Vector(bb.XMin-dx, bb.YMax+dy, 0)
            NE = App.Vector(bb.XMax+dx, bb.YMax+dy, 0)
            SE = App.Vector(bb.XMax+dx, bb.YMin-dy, 0)
            edge0 = Part.makeLine(SO, NO)
            edge1 = Part.makeLine(NO, NE)
            edge2 = Part.makeLine(NE, SE)
            edge3 = Part.makeLine(SE, SO)
            ground_face = Part.makeFace([ Part.Wire([edge0, edge1, edge2, edge3]) ])

            ground =  App.ActiveDocument.addObject("Mesh::Feature", "Ground")
            ground.Mesh = MeshPart.meshFromShape(Shape=ground_face, LinearDeflection=0.1, AngularDeflection=0.523599, Relative=False)
            ground.Label = "Ground"
            self.set_property(ground, "App::PropertyString", "shType", "The element type", 'ground')
            self.set_property(ground, "App::PropertyString", "id", "The ground's id", 'ground')


        set_color_and_transparency(ground, self.site.groundColor)
        ground.ViewObject.Transparency = 50

        self.add_fc_objects(ground)

        self.site.addObject(ground)

    def _import_elements(self, parent, xpath):
        """Generic function to import a specific element.

        This function will lookup the handler registered for the elements
        `tag` and then call it on each item. It also provides some update
        on the whole process.

        Args:
            parent (Element): the parent of the elements to be imported.
                Usually the <home> element.
            xpath (str): the xpath of the elements to be imported.
            update_progress (bool, optional): whether to update the
                progress. Set to false when importing a group of elements.
                Defaults to True.
        """
        elements = parent.findall(xpath)
        # Is it a real tag name or an xpath expression?
        tag_name = xpath[3:] if xpath.startswith('.') else xpath
        total_steps, current_step = self._get_progress_info(xpath)
        total_elements = len(elements)
        if self.progress_bar:
            self.progress_bar.stop()
            self.progress_bar.start(f"Step {current_step}/{total_steps}: importing {total_elements} '{tag_name}' elements. Standby…", total_elements)
        _msg(f"Importing {total_elements} '{tag_name}' elements…")
        handler = self.handlers[xpath]
        def _process(tuple):
            (i, elm) = tuple
            _msg(f"Importing {tag_name}#{i} ({self.current_object_count + 1}/{self.total_object_count})…")
            try:
                # with Transaction(f"Importing {tag_name}#{i}"):
                    handler.process(parent, i, elm)
            except Exception as e:
                _err(f"Importing {tag_name}#{i} failed")
                _err(str(e))
                _err(traceback.format_exc())

            if self.progress_bar: self.progress_bar.next()
            self.current_object_count = self.current_object_count + 1
        list(map(_process, enumerate(elements)))

    def _get_progress_info(self, xpath):
        xpaths = list(self.handlers.keys())
        total_steps = len(xpaths)
        current_step = xpaths.index(xpath)+1
        return total_steps, current_step

    def _set_site_properties(self, elm):
        # All information in environment?, backgroundImage?, print?, compass
        # are added to the site object. Some are furthermore added to the ground
        environments = elm.findall('environment')
        if len(environments) > 0:
            environment = environments[0]
            ground_color = environment.get('groundColor',self.preferences["DEFAULT_GROUND_COLOR"])
            sky_color = environment.get('ceilingColor', self.preferences["DEFAULT_SKY_COLOR"])
            lightColor = environment.get('lightColor', self.preferences["DEFAULT_SKY_COLOR"])
            ceillingLightColor = environment.get('ceillingLightColor', self.preferences["DEFAULT_SKY_COLOR"])

            self.set_property(self.site, "App::PropertyString", "groundColor", "", ground_color)
            self.set_property(self.site, "App::PropertyBool", "backgroundImageVisibleOnGround3D", "", environment)
            self.set_property(self.site, "App::PropertyString", "skyColor", "", sky_color)
            self.set_property(self.site, "App::PropertyString", "lightColor", "", lightColor)
            self.set_property(self.site, "App::PropertyFloat", "wallsAlpha", "", environment)
            self.set_property(self.site, "App::PropertyBool", "allLevelsVisible", "", environment)
            self.set_property(self.site, "App::PropertyBool", "observerCameraElevationAdjusted", "", environment)
            self.set_property(self.site, "App::PropertyString", "ceillingLightColor", "", ceillingLightColor)
            self.set_property(self.site, "App::PropertyEnumeration", "drawingMode", "", str(environment.get('drawingMode', 'FILL')), valid_values=["FILL", "OUTLINE", "FILL_AND_OUTLINE"])
            self.set_property(self.site, "App::PropertyFloat", "subpartSizeUnderLight", "", environment)
            self.set_property(self.site, "App::PropertyInteger", "photoWidth", "", environment)
            self.set_property(self.site, "App::PropertyInteger", "photoHeight", "", environment)
            self.set_property(self.site, "App::PropertyEnumeration", "photoAspectRatio", "", str(environment.get('photoAspectRatio', 'VIEW_3D_RATIO')), valid_values=["FREE_RATIO", "VIEW_3D_RATIO", "RATIO_4_3", "RATIO_3_2", "RATIO_16_9", "RATIO_2_1", "RATIO_24_10", "SQUARE_RATIO"])
            self.set_property(self.site, "App::PropertyInteger", "photoQuality", "", environment)
            self.set_property(self.site, "App::PropertyInteger", "videoWidth", "", environment)
            self.set_property(self.site, "App::PropertyEnumeration", "videoAspectRatio", "", str(environment.get('videoAspectRatio', 'RATIO_4_3')), valid_values=["RATIO_4_3", "RATIO_16_9", "RATIO_24_10"])
            self.set_property(self.site, "App::PropertyInteger", "photoQuality", "", environment)
            self.set_property(self.site, "App::PropertyInteger", "videoQuality", "", environment)
            self.set_property(self.site, "App::PropertyString", "videoSpeed", "", environment)
            self.set_property(self.site, "App::PropertyInteger", "videoFrameRate", "", environment)
        else:
            _msg(f"No <environment> tag found in <{elm.tag}>")

        bg_imgs = elm.findall('backgroundImage')
        if len(bg_imgs) > 0:
            bg_img = bg_imgs[0]
            self.set_property(self.site, "App::PropertyString", "image", "", bg_img)
            self.set_property(self.site, "App::PropertyFloat", "scaleDistance", "", bg_img)
            self.set_property(self.site, "App::PropertyFloat", "scaleDistanceXStart", "", bg_img)
            self.set_property(self.site, "App::PropertyFloat", "scaleDistanceYStart", "", bg_img)
            self.set_property(self.site, "App::PropertyFloat", "scaleDistanceXEnd", "", bg_img)
            self.set_property(self.site, "App::PropertyFloat", "scaleDistanceYEnd", "", bg_img)
            self.set_property(self.site, "App::PropertyFloat", "xOrigin", "", bg_img)
            self.set_property(self.site, "App::PropertyFloat", "yOrigin", "", bg_img)
            self.set_property(self.site, "App::PropertyBool", "visible", "Whether the background image is visible", bg_img)
        else:
            _msg(f"No <backgroundImage> tag found in <{elm.tag}>")

        compasses = elm.findall('compass')
        if len(compasses) > 0:
            compass = compasses[0]
            self.set_property(self.site, "App::PropertyFloat", "x", "The compass's x", compass)
            self.set_property(self.site, "App::PropertyFloat", "y", "The compass's y", compass)
            self.set_property(self.site, "App::PropertyFloat", "diameter", "The compass's diameter in cm", compass)
            self.set_property(self.site, "App::PropertyFloat", "northDirection", "The compass's angle to the north in degree", compass)
            self.set_property(self.site, "App::PropertyFloat", "longitude", "The compass's longitude", compass)
            self.set_property(self.site, "App::PropertyFloat", "latitude", "The compass's latitude", compass)
            self.set_property(self.site, "App::PropertyString", "timeZone", "The compass's TimeZone", compass)
            self.set_property(self.site, "App::PropertyBool", "visible", "Whether the compass is visible", compass)
            self.site.Declination = math.degrees(ang_sh2fc(float(self.site.northDirection)))
            self.site.Longitude = math.degrees(float(self.site.longitude))
            self.site.Latitude = math.degrees(float(self.site.latitude))
            self.site.EPWFile = '' # https://www.ladybug.tools/epwmap/ or https://climate.onebuilding.org
        else:
            _msg(f"No <compass> tag found in <{elm.tag}>")

    def _create_slabs(self):
        floors = self.floors.values()
        all_walls = self.get_all_walls()
        all_spaces = self.get_all_spaces()
        total_steps, current_step = self._get_progress_info(ET_XPATH_DUMMY_SLAB)
        total_elements = len(floors)
        if self.progress_bar:
            self.progress_bar.stop()
            self.progress_bar.start(f"Step {current_step}/{total_steps}: Creating {total_elements} 'slab' elements. Standby…", len(all_walls) + len(all_spaces))

        _msg(f"Creating {total_elements} 'slab' elements…")
        handler = self.handlers[ET_XPATH_LEVEL]
        def _create_slab(tuple):
            (i, floor) = tuple
            _msg(f"Creating slab#{i} for floor '{floor.Label}'…")
            try:
                # with Transaction(f"Creating slab#{i} for floor '{floor.Label}'"):
                    handler.create_slabs(floor, self.progress_bar)
            except Exception as e:
                _err(f"Creating slab#{i} for floor '{floor.Label}' failed")
                _err(str(e))
                _err(traceback.format_exc())
        list(map(_create_slab, enumerate(floors)))

    def _decorate_surfaces(self):

        all_walls = self.get_all_walls()
        all_spaces = self.get_all_spaces()

        total_elements = len(all_spaces)+len(all_walls)

        if self.progress_bar:
            self.progress_bar.stop()
            self.progress_bar.start(f"Decorating {total_elements} elements. Standby…", total_elements)
        _msg(f"Decorating {total_elements} elements…")

        handler = self.handlers[ET_XPATH_ROOM]
        for i, space in enumerate(all_spaces):
            handler.post_process(space)
            if self.progress_bar: self.progress_bar.next()

        handler = self.handlers[ET_XPATH_WALL]
        for i, wall in enumerate(all_walls):
            handler.post_process(wall)
            if self.progress_bar: self.progress_bar.next()
        if self.progress_bar: self.progress_bar.stop()


class BaseHandler:
    """The base class for all importers."""

    def __init__(self, importer: SH3DImporter):
        self.importer = importer

    def setp(self, obj, type_, name, description, value=None, valid_values=None):
        """Set a property on the object

        Args:
            obj (FreeCAD): the object on which to set the property
            type_ (str): the property type
            name (str): the property name
            description (str): the property description
            value (xml.etree.ElementTree.Element|str, optional): The
                property's value. Defaults to None.
            valid_values (list, optional): The property's enumerated values.
                Defaults to None.
        """
        self.importer.set_property(obj, type_, name, description, value, valid_values)

    def get_fc_object(self, id, sh_type):
        """Returns the FC object with the specified id and sh_type

        Args:
            id (str): the id of the element to lookup
            sh_type (str, optional): The SweetHome type of the element to be
                imported. Defaults to None.

        Returns:
            FCObject: The FC object that correspond to the imported SH element
        """
        return self.importer.get_fc_object(id, sh_type)

    def get_floor(self, level_id):
        """Returns the Floor associated with the level_id.

        Returns the first level if there is just one level or if level_id is
            None

        Args:
            levels (list): The list of imported levels
            level_id (string): the level @id

        Returns:
            level: The level
        """
        return self.importer.get_floor(level_id)

    def get_spaces(self, floor):
        return self.importer.get_spaces(floor)

    def get_space(self, floor, p):
        """Returns the Space this point belongs to.

        An point belongs to a space if it is the closest space below that point

        Args:
            floor (level): the floor the point's parent belongs to.
            p (Point): the point for which to determine the closest space.

        Returns:
            Space: the space the object belongs to or None
        """
        return self.importer.get_space(floor, p)

    def get_walls(self, floor):
        return self.importer.get_walls(floor)

    def get_wall_spine(self, wall):
        if not hasattr(wall, 'BaseObjects'):
            _err(f"Wall {wall.Label} has no BaseObjects to get the Spine from…")
        return wall.BaseObjects[2]

    def get_faces(self, wall):
        """Returns the name of the left and right face for `wall`

        The face names are suitable for selection later on when creating
        the Facebinders and baseboards. Note, that this must be executed
        once the wall has been completely constructed. If a window
        or door is added afterward, this will have an impact on what is
        considered the left and right side of the wall

        Args:
            wall (Arch.Wall): the wall for which we have to determine
                the left and right side.

        Returns:
            tuple: a tuple of string containing the name of the left and
                right side of the wall
        """
        # In order to handle curved walls, take the oriented line (from
        # start to end) that pass through the center of gravity of the wall
        # Hopefully the COG of the face will always be on the correct side
        # of the COG of the wall
        wall_spine = self.get_wall_spine(wall)
        wall_start = wall_spine.Start
        wall_end = wall_spine.End
        wall_cog_start = wall.Shape.CenterOfGravity
        wall_cog_end = wall_cog_start + wall_end - wall_start

        left_face_name = right_face_name = None
        left_face = right_face = None
        for (i, face) in enumerate(wall.Shape.Faces):
            face_cog = face.CenterOfGravity

            # The face COG is not on the same z as the wall COG
            # just skipping.
            if not math.isclose(face_cog.z, wall_cog_start.z, abs_tol=1):
                continue

            side = self._get_face_side(wall_cog_start, wall_cog_end, face_cog)
            # NOTE: face names start at 1...
            if side > 0:
                left_face_name = f"Face{i+1}"
                left_face = face
            elif side < 0:
                right_face_name = f"Face{i+1}"
                right_face = face
            if left_face_name and right_face_name:
                # Optimization. Is it always true?
                break
        return (left_face_name, left_face, right_face_name, right_face)

    def _get_face_side(self, start:App.Vector, end:App.Vector, cog:App.Vector):
        # Compute vectors
        ab = end - start  # Vector from start to end
        ac = cog - start  # Vector from start to CenterOfGravity

        ab.z = 0
        ac.z = 0

        # Compute the cross product (z-component is enough for 2D test)
        cross_z = ab.x * ac.y - ab.y * ac.x

        # Determine the position of point cog
        if math.isclose(cross_z, 0, abs_tol=1):
            return 0
        return cross_z

    def _ps(self, section, print_z: bool = False):
        # Pretty print a Section in a condensed way
        if hasattr(section, 'Shape'):
            v = section.Shape.Vertexes
        else:
            # a Part.Face
            v = section.Vertexes
        return f"[{self._pv(v[0].Point, print_z)}, {self._pv(v[1].Point, print_z)}, {self._pv(v[2].Point, print_z)}, {self._pv(v[3].Point, print_z)}]"

    def _pe(self, edge, print_z: bool = False):
        # Print an Edge in a condensed way
        v = edge.Vertexes
        return f"[{self._pv(v[0].Point, print_z)}, {self._pv(v[1].Point, print_z)}]"

    def _pes(self, edges, print_z: bool = False):
        return '->'.join(list(map(lambda e: self._pe(e, print_z), edges)))

    def _pv(self, v, print_z: bool = False, ndigits: None = None):
        # Print an Vector in a condensed way
        if not v:
            return "NaN"
        if hasattr(v,'X'):
            return f"({round(getattr(v, 'X'), ndigits)},{round(getattr(v, 'Y'), ndigits)}{',' + str(round(getattr(v, 'Z'), ndigits) if print_z else '')})"
        elif hasattr(v,'x'):
            return f"({round(getattr(v, 'x'), ndigits)},{round(getattr(v, 'y'), ndigits)}{',' + str(round(getattr(v, 'z'), ndigits) if print_z else '')})"
        raise ValueError(f"Expected a Point or Vector, got {type(v)}")

    def _debug_point(self, coord, label, color=RED):
        part = Draft.make_point(coord)
        part.Label = label
        part.ViewObject.PointSize = 5
        part.ViewObject.PointColor = color
        App.ActiveDocument.DEBUG_GEOMETRY.addObject(part)
        return part

    def _debug_vector(self, vector, label, color=RED, placement=None):
        part = Draft.make_line(ORIGIN, vector)
        if placement:
            part.Placement = placement
        part.Label = label
        part.ViewObject.LineWidth = 5
        part.ViewObject.LineColor = color
        App.ActiveDocument.DEBUG_GEOMETRY.addObject(part)
        return part

    def _debug_shape(self, shape, label, color=GREEN, transparency=.75, placement=None):
        part = Part.show(shape)
        if placement:
            part.Placement = placement
        part.Label = label
        part.ViewObject.LineColor = color
        part.ViewObject.PointSize = 5
        material = part.ViewObject.ShapeAppearance[0]
        material.DiffuseColor = color
        material.Transparency = transparency
        part.ViewObject.ShapeAppearance = (material)
        App.ActiveDocument.DEBUG_GEOMETRY.addObject(part)
        return part

    def _debug_mesh(self, mesh, label, transform=None, color=GREEN, transparency=.75, placement=None):
        shape = Part.Shape()
        new_mesh = mesh.copy()
        if transform:
            new_mesh.transform(transform)
        shape.makeShapeFromMesh(new_mesh.Topology, 1)
        self._debug_shape(shape, label, color, transparency, placement)


class LevelHandler(BaseHandler):
    """A helper class to import a SH3D `<level>` object."""

    def __init__(self, importer: SH3DImporter):
        super().__init__(importer)

    def process(self, parent, i, elm):
        """Creates and returns a Arch::Floor

        Args:
            i (int): the ordinal of the imported element
            elm (Element): the xml element
        """
        floor = None
        if self.importer.preferences["MERGE"]:
            floor = self.get_fc_object(elm.get("id"), 'level')

        if not floor:
            floor = Arch.makeFloor()

        floor.Label = elm.get('name')
        floor.Placement.Base.z = dim_sh2fc(float(elm.get('elevation')))
        floor.Height = dim_sh2fc(float(elm.get('height')))
        self._set_properties(floor, elm)
        floor.Visibility = elm.get('visible', 'true') == 'true'
        self._create_groups(floor)
        self.importer.add_floor(floor)

    def create_default_floor(self):
        floor = Arch.makeFloor()
        floor.Label = 'Level'
        floor.Placement.Base.z = 0
        floor.Height = 2500

        self._set_properties(floor, dict({'shType': 'level', 'id':'Level', 'floorThickness':25, 'elevationIndex': 0, 'viewable': True}))
        self._create_groups(floor)
        self.importer.add_floor(floor)

        return floor

    def _set_properties(self, obj, elm):
        self.setp(obj, "App::PropertyString", "shType", "The element type", 'level')
        self.setp(obj, "App::PropertyString", "id", "The floor's id", elm)
        self.setp(obj, "App::PropertyQuantity", "floorThickness", "The floor's slab thickness", dim_sh2fc(float(elm.get('floorThickness'))))
        self.setp(obj, "App::PropertyInteger", "elevationIndex", "The floor number", elm)
        self.setp(obj, "App::PropertyBool", "viewable", "Whether the floor is viewable", elm)

    def _create_groups(self, floor):
        # This is a special group that does not appear in the TreeView.
        group = self._create_group(floor, "ReferenceFacesGroupName", f"References-{floor.Label}")
        if not self.importer.preferences["DEBUG_GEOMETRY"]:
            group.Visibility = False
            group.ViewObject.ShowInTree = False

        group = self._create_group(floor, "SlabObjectsGroupName", f"SlabObjects-{floor.Label}")
        if not self.importer.preferences["DEBUG_GEOMETRY"]:
            group.Visibility = False
            group.ViewObject.ShowInTree = False

        if self.importer.preferences["DECORATE_SURFACES"]:
            self._create_group(floor, "DecorationWallsGroupName", f"Decoration-{floor.Label}-Walls")
            self._create_group(floor, "DecorationCeilingsGroupName", f"Decoration-{floor.Label}-Ceilings")
            self._create_group(floor, "DecorationFloorsGroupName", f"Decoration-{floor.Label}-Floors")
            self._create_group(floor, "DecorationBaseboardsGroupName", f"Decoration-{floor.Label}-Baseboards")

        if self.importer.preferences["IMPORT_FURNITURES"]:
            self._create_group(floor, "FurnitureGroupName", f"Furnitures-{floor.Label}")

        if self.importer.preferences["IMPORT_LIGHTS"]:
            self._create_group(floor, "LightGroupName", f"Lights-{floor.Label}")

    def _create_group(self, floor, prop_group_name, group_label):
        group = None
        if self.importer.preferences["MERGE"]:
            if hasattr(floor, prop_group_name):
                group_name = getattr(floor, prop_group_name)
                group = floor.getObject(group_name)

        if not group:
            group = floor.newObject("App::DocumentObjectGroup")
            group.Label = group_label
            self.setp(floor, "App::PropertyString", prop_group_name, "The DocumentObjectGroup name for the group on this floor", group.Name)

        return group

    def create_slabs(self, floor, progress_bar):
        """Creates a Arch.Slab for the given floor.

        Creating a slab consists in projecting all the structures of that
        floor into a plane, then create a extrusion for each one and then
        fuse thogether (in order to simplify the slab geometry).

        Args:
            floor (Arch.Floor): the Arch Floor for which to create the Slab
        """
        slab = None
        if self.importer.preferences["MERGE"]:
            slab = self.get_fc_object(f"{floor.id}-slab", 'slab')

        def _extrude(obj_to_extrude):
            """Return the Part.Extrude suitable for fusion by the make_multi_fuse tool.

            Args:
                floor (Arch.Floor): the Arch Floor for which to create the Slab
                obj_to_extrude (Part): the space or wall to project onto the XY
                plane to create the slab

            Returns:
                Part.Feature: the extrusion used to later to fuse.
            """
            if self.importer.preferences["DEBUG_GEOMETRY"]:
                _log(f"Extruding {obj_to_extrude.Label}…")
            obj_to_extrude.recompute(True)
            projection = TechDraw.project(obj_to_extrude.Shape, Z_NORM)[0]
            face = Part.Face(Part.Wire(projection.Edges))
            extrude = face.extrude(-Z_NORM*floor.floorThickness.Value)
            part = Part.show(extrude, "Extrusion")
            # part.Placement.Base.z = floor.Placement.Base.z
            part.Label = f"{floor.Label}-{obj_to_extrude.Label}-extrusion"
            part.recompute(True)
            part.Visibility = False
            part.ViewObject.ShowInTree = False

            if progress_bar:
                progress_bar.next()

            return part

        if not slab:
            # Take the spaces whose floor is actually visible, and all the walls
            projections = list(map(lambda s: s.ReferenceFace, filter(lambda s: s.floorVisible, self.get_spaces(floor))))
            projections.extend(list(map(lambda w: w.ReferenceFace, self.get_walls(floor))))
            extrusions = list(map(_extrude, projections))
            extrusions = list(filter(lambda o: o is not None, extrusions))
            if len(extrusions) > 0:
                if len(extrusions) > 1:
                    bf = BOPTools.BOPFeatures.BOPFeatures(App.ActiveDocument)
                    slab_base = bf.make_multi_fuse([ o.Name for o in extrusions])
                    slab_base.Label = f"{floor.Label}-footprint"
                    slab_base.recompute()
                else:
                    slab_base = extrusions[0]
                    slab_base.Label = f"{floor.Label}-footprint"

                slab = Arch.makeStructure(slab_base)
                slab.Placement.Base.z = floor.Placement.Base.z
                slab.Normal = -Z_NORM
                slab.setExpression('Height', f"{slab_base.Name}.Shape.BoundBox.ZLength")
            else:
                _wrn(f"No object found for floor {floor.Label}.")
                self.setp(floor, "App::PropertyString", "ReferenceSlabName", "The name of the Slab used on this floor", None)
                return

            slab.Label = f"{floor.Label}-slab"

            if self.importer.preferences["DEBUG_GEOMETRY"]:
                slab.ViewObject.DisplayMode = 'Wireframe'
                slab.ViewObject.DrawStyle = 'Dotted'
                slab.ViewObject.LineColor = ORANGE
                slab.ViewObject.LineWidth = 2

        self.setp(slab, "App::PropertyString", "shType", "The element type", 'slab')
        self.setp(slab, "App::PropertyString", "id", "The slab's id", f"{floor.id}-slab")
        self.setp(slab, "App::PropertyString", "ReferenceFloorName", "The name of the Arch.Floor this slab belongs to", floor.Name)
        self.setp(floor, "App::PropertyString", "ReferenceSlabName", "The name of the Slab used on this floor", slab.Name)

        floor.addObject(slab)


class RoomHandler(BaseHandler):
    """A helper class to import a SH3D `<room>` object.

    It also handles the <point> elements found as children of the <room> element.
    """

    def __init__(self, importer: SH3DImporter):
        super().__init__(importer)

    def process(self, parent, i, elm):
        """Creates and returns a Arch::Structure from the imported_room object

        Args:
            i (int): the ordinal of the imported element
            elm (Element): the xml element
        """
        debug_geometry = self.importer.preferences["DEBUG_GEOMETRY"]

        level_id = elm.get('level', None)
        floor = self.get_floor(level_id)
        assert floor != None, f"Missing floor '{level_id}' for <room> '{elm.get('id')}'…"

        space = face = None
        if self.importer.preferences["MERGE"]:
            space = self.get_fc_object(elm.get("id"), 'room')

        # A Room is composed of a space with a Face as the base object
        if not space:
            name = elm.get('name', 'Room')

            floor_z = dim_fc2sh(floor.Placement.Base.z)
            points = [ coord_sh2fc(App.Vector(float(p.get('x')), float(p.get('y')), floor_z)) for p in elm.findall('point') ]
            # remove consecutive identical points
            points = [points[i] for i in range(len(points)) if i == 0 or points[i] != points[i - 1]]
            # and close the wire
            points.append(points[0])
            # Offset to avoid self-intersecting wires
            reference_wire = Part.makePolygon(points)
            if debug_geometry: self._debug_shape(reference_wire, f"{name}-reference-wire", RED)
            reference_wire = self._get_offset_wire(reference_wire)
            if debug_geometry: self._debug_shape(reference_wire, f"{name}-reference-wire-offset", RED)
            points = [v.Point for v in reference_wire.Vertexes]
            reference_face = Draft.make_wire(points, closed=True, face=True, support=None)
            reference_face.Label = f"{name}-reference"
            reference_face.Visibility = False
            reference_face.recompute()

            floor.getObject(floor.ReferenceFacesGroupName).addObject(reference_face)

            # NOTE: for room to properly display and calculate the area, the
            # Base object can not be a face but must have a height...
            footprint = App.ActiveDocument.addObject("Part::Feature", "Footprint")
            footprint.Shape = reference_face.Shape.extrude(Z_NORM)
            footprint.Label = f"{name}-footprint"
            self.setp(footprint, "App::PropertyLink", "ReferenceFace", "The Reference Part.Wire", reference_face)

            space = Arch.makeSpace(footprint)
            space.IfcType = "Space"
            space.Label = name
            self._set_properties(space, elm)

            space.setExpression('ElevationWithFlooring', f"{footprint.Name}.Shape.BoundBox.ZMin")
            self.setp(space, "App::PropertyString", "ReferenceFloorName", "The name of the Arch.Floor this room belongs to", floor.Name)
            self.setp(space, "App::PropertyLink", "ReferenceFace", "The Reference Part.Wire", reference_face)

        self.importer.add_space(floor, space)

        space.Visibility = True if space.floorVisible else False

        floor.addObject(space)

    def _set_properties(self, obj, elm):
        floor_color = elm.get('floorColor',self.importer.preferences["DEFAULT_FLOOR_COLOR"])
        ceiling_color = elm.get('ceilingColor', self.importer.preferences["DEFAULT_CEILING_COLOR"])

        self.setp(obj, "App::PropertyString", "shType", "The element type", 'room')
        self.setp(obj, "App::PropertyString", "id", "The slab's id", elm.get('id', str(uuid.uuid4())))
        self.setp(obj, "App::PropertyFloat", "nameAngle", "The room's name angle", elm)
        self.setp(obj, "App::PropertyFloat", "nameXOffset", "The room's name x offset", elm)
        self.setp(obj, "App::PropertyFloat", "nameYOffset", "The room's name y offset", elm)
        self.setp(obj, "App::PropertyBool", "areaVisible", "Whether the area of the room is displayed in the plan view", elm)
        self.setp(obj, "App::PropertyFloat", "areaAngle", "The room's area annotation angle", elm)
        self.setp(obj, "App::PropertyFloat", "areaXOffset", "The room's area annotation x offset", elm)
        self.setp(obj, "App::PropertyFloat", "areaYOffset", "The room's area annotation y offset", elm)
        self.setp(obj, "App::PropertyBool", "floorVisible", "Whether the floor of the room is displayed", elm)
        self.setp(obj, "App::PropertyString", "floorColor", "The room's floor color", floor_color)
        self.setp(obj, "App::PropertyPercent", "floorShininess", "The room's floor shininess", percent_sh2fc(elm.get('floorShininess', 0)))
        self.setp(obj, "App::PropertyBool", "ceilingVisible", "Whether the ceiling of the room is displayed", elm)
        self.setp(obj, "App::PropertyString", "ceilingColor", "The room's ceiling color", ceiling_color)
        self.setp(obj, "App::PropertyPercent", "ceilingShininess", "The room's ceiling shininess", percent_sh2fc(elm.get('ceilingShininess', 0)))
        self.setp(obj, "App::PropertyBool", "ceilingFlat", "", elm)

    def _get_offset_wire(self, wire, inward=True):
        """Return an inward (or outward) offset wire to avoid self intersection.

        This will return a non self-intersecting wire offsetted either inward
        or outward from the original wire.

        Args:
            wire (Part.Wire): the original self-intersecting wire.

        Returns:
            Part.Wire: a non self intersecting wire
        """
        edges = wire.Edges
        self_intersect = self._self_intersect(edges)
        if not self_intersect:
            return wire

        offset_wire = wire.copy()
        offset_vector = self._get_offset_vector(edges[0], inward)
        multiplier = 1
        while self_intersect and multiplier < 5:
            # Self intersecting wire can not be properly extruded to
            # create rooms. We offset the wire inward until it stop
            # self intersecting.
            offset_wire = DraftGeomUtils.offsetWire(wire, offset_vector*multiplier)
            self_intersect =  self._self_intersect(offset_wire.Edges)
            multiplier += 1
        else:
            if self_intersect:
                return self._get_offset_wire(wire, False)
        return offset_wire

    def _self_intersect(self, edges):
        """Returns whether a list of edges self intersect.

        Returns True if at least one pair of edge intersect.

        Args:
            edges (list): list of Part.Edge to test

        Returns:
            bool: True if at least one pair of edge intersect.
            list(tuple): a list of tuple of v1, e1, v2, e2 where v1 is the
                intersection on the first edge e1, and v2 is the intersection
                on the second edge e2.
        """
        for i in range(len(edges)):
            for j in range(i + 1, len(edges)):  # Avoid duplicate checks
                e1 = edges[i]
                e2 = edges[j]
                (dist, vectors, _) = e1.distToShape(e2)
                if dist > 0:
                    continue
                for (v1, v2) in vectors:
                    # Check that the intersections are not extremities
                    # If both v1 and v2 are extremities then the edges
                    # are connected which is not really a self-intersecting
                    # situation.
                    if v1 not in [v.Point for v in e1.Vertexes] or v2 not in [v.Point for v in e2.Vertexes]:
                        return True
        return False

    def _get_offset_vector(self, edge, inward):
        """Returns the normal vector at start, either inward facing or outward.

        Args:
            edge (Part.Edge): The edge for which to find the normal at
            inward (bool): whether to get take the cross or the inverse.

        Returns:
            Vector: the normal vector
        """
        tangent = edge.tangentAt(edge.FirstParameter).normalize()
        return App.Rotation(Z_NORM, -90 if inward else 90).multVec(tangent)

    def post_process(self, obj):
        if self.importer.preferences["DECORATE_SURFACES"]:
            floor = App.ActiveDocument.getObject(obj.ReferenceFloorName)
            self._add_facebinder(floor, obj, "floor")
            self._add_facebinder(floor, obj, "ceiling")

    def _add_facebinder(self, floor, space, side):
        facebinder_id = f"{floor.id}-{space.id}-{side}-facebinder"
        facebinder = None
        if self.importer.preferences["MERGE"]:
            facebinder = self.get_fc_object(facebinder_id, 'facebinder')

        if not facebinder:
            # NOTE: always use Face1 as this is a 2D object
            facebinder = Draft.make_facebinder(( space.ReferenceFace, ("Face1", ) ))
            facebinder.Extrusion = 1
            facebinder.Label = space.Label + f" {side} finish"

        facebinder.Placement.Base.z = 1 if (side == "floor") else floor.Height.Value-1
        facebinder.Visibility = getattr(space, f"{side}Visible")
        set_color_and_transparency(facebinder, getattr(space, f"{side}Color"))
        set_shininess(facebinder, getattr(space, f"{side}Shininess", 0))

        self.setp(facebinder, "App::PropertyString", "shType", "The element type", 'facebinder')
        self.setp(facebinder, "App::PropertyString", "id", "The element's id", facebinder_id)
        self.setp(facebinder, "App::PropertyString", "ReferenceRoomName", "The Reference Arch.Space", space.Name)

        group_name = getattr(floor, "DecorationFloorsGroupName") if (side == "floor") else getattr(floor, "DecorationCeilingsGroupName")
        floor.getObject(group_name).addObject(facebinder)


class WallHandler(BaseHandler):
    """A helper class to import a SH3D `<wall>` object."""

    def __init__(self, importer: SH3DImporter):
        super().__init__(importer)
        self.wall_sections = {}

    def process(self, parent, i, elm):
        """Creates and returns a Arch::Structure from the imported_wall object

        Args:
            parent (Element): the parent Element of the wall to be imported
            i (int): the ordinal of the imported element
            elm (Element): the xml element
        """
        level_id = elm.get('level', None)
        floor = self.get_floor(level_id)
        assert floor != None, f"Missing floor '{level_id}' for <wall> '{elm.get('id')}'…"

        wall = base_object = None
        if self.importer.preferences["MERGE"]:
            wall = self.get_fc_object(elm.get("id"), 'wall')

        if not wall:
            prev = self._get_sibling_wall(parent, elm, 'wallAtStart')
            next = self._get_sibling_wall(parent, elm, 'wallAtEnd')
            wall, base_object = self._create_wall(floor, prev, next, elm)
            if not wall:
                _log(f"No wall created for {elm.get('id')}. Skipping!")
                return

        wall.IfcType = "Wall"
        wall.Label = f"wall{i}"
        wall.Base.Label = f"wall{i}-volume"
        wall.BaseObjects[0].Label = f"wall{i}-start"
        wall.BaseObjects[1].Label = f"wall{i}-end"
        wall.BaseObjects[2].Label = f"wall{i}-spine"
        self._set_properties(wall, elm)
        self._set_baseboard_properties(wall, elm)
        self.setp(wall, "App::PropertyString", "ReferenceFloorName", "The Name of the Arch.Floor this walls belongs to", floor.Name)

        wall.recompute(True)

        floor.addObject(wall)
        if base_object:
            floor.addObject(base_object)
            base_object.Visibility = False
            base_object.Label = base_object.Label + "-" + wall.Label

        self.importer.add_wall(floor, wall)

    def _get_sibling_wall(self, parent, wall, sibling_attribute_name):
        sibling_wall_id = wall.get(sibling_attribute_name, None)
        if not sibling_wall_id:
            return None
        sibling_wall = parent.find(f"./wall[@id='{sibling_wall_id}']")
        if sibling_wall is None:
            wall_id = wall.get('id')
            raise ValueError(f"Invalid SweetHome3D file: wall {wall_id} reference an unknown wall {sibling_wall_id}")
        return sibling_wall

    def _set_properties(self, obj, elm):

        top_color = elm.get('topColor', self.importer.preferences["DEFAULT_FLOOR_COLOR"])
        left_side_color = elm.get('leftSideColor', self.importer.preferences["DEFAULT_FLOOR_COLOR"])
        right_side_color = elm.get('rightSideColor', self.importer.preferences["DEFAULT_FLOOR_COLOR"])

        self.setp(obj, "App::PropertyString", "shType", "The element type", 'wall')
        self.setp(obj, "App::PropertyString", "id", "The wall's id", elm)
        self.setp(obj, "App::PropertyString", "wallAtStart", "The Id of the contiguous wall at the start of this wall", elm)
        self.setp(obj, "App::PropertyString", "wallAtEnd", "The Id of the contiguous wall at the end of this wall", elm)
        self.setp(obj, "App::PropertyString", "pattern", "The pattern of this wall in plan view", elm)
        self.setp(obj, "App::PropertyString", "topColor", "The wall inner color", top_color)
        self.setp(obj, "App::PropertyString", "leftSideColor", "The wall inner color", left_side_color)
        self.setp(obj, "App::PropertyPercent","leftSideShininess", "The room's ceiling shininess", percent_sh2fc(elm.get('leftSideShininess', 0)))
        self.setp(obj, "App::PropertyString", "rightSideColor", "The wall inner color", right_side_color)
        self.setp(obj, "App::PropertyPercent","rightSideShininess", "The room's ceiling shininess", percent_sh2fc(elm.get('rightSideShininess', 0)))

    def _set_baseboard_properties(self, obj, elm):
        # Baseboard are a little bit special:
        # Since their placement and other characteristics are dependent on
        # the wall elements to be created (such as doorOrWndows), their
        # creation is delayed until then
        for baseboard in elm.findall('baseboard'):
            side = baseboard.get('attribute')
            self.setp(obj, "App::PropertyQuantity", f"{side}Thickness", f"The thickness of the {side} baseboard", dim_sh2fc(float(baseboard.get("thickness"))))
            self.setp(obj, "App::PropertyQuantity", f"{side}Height", f"The height of the {side} baseboard", dim_sh2fc(float(baseboard.get("height"))))
            self.setp(obj, "App::PropertyString", f"{side}Color", f"The color of the {side} baseboard", baseboard.get("color"))

    def _create_wall(self, floor, prev, next, elm):
        """Create an Arch::Structure from an SH3D Element.

        The constructed wall will either be a straight wall or a curved
        wall depending on the `elm` attributes.

        Args:
            floor (Arch::Structure): The floor the wall belongs to
            prev (Element): the xml element for the previous sibling wall
            next (Element): the xml element for the next sibling wall
            elm (Element): the xml element for the wall to be imported

        Returns:
            Arch::Wall: the newly created wall
        """
        debug_geometry = self.importer.preferences["DEBUG_GEOMETRY"]

        wall_details = self._get_wall_details(floor, elm)
        assert wall_details is not None, f"Fail to get details of wall {elm.get('id')}. Bailing out! {elm} / {wall_details}"

        # Both the wall at start or the wall at end can be None.
        prev_wall_details = self._get_wall_details(floor, prev)
        next_wall_details = self._get_wall_details(floor, next)

        is_wall_straight = wall_details[5] == 0

        # Is the wall curved (i.e. arc_extent != 0) ?
        if is_wall_straight:
            section_start, section_end, spine = self._create_straight_segment(
                wall_details,
                prev_wall_details,
                next_wall_details)
        else:
            section_start, section_end, spine = self._create_curved_segment(
                wall_details,
                prev_wall_details,
                next_wall_details)

        base_object = None
        App.ActiveDocument.recompute([section_start, section_end, spine])
        if debug_geometry:
            _log(f"_create_wall(): wall => section_start={self._ps(section_start)}, section_end={self._ps(section_end)}")

        sweep = self._make_sweep(section_start, section_end, spine)
        # Sometimes the Part::Sweep creates a "twisted" sweep which
        # result in a broken wall. The solution is to use a compound
        # object based on ruled surface instead.
        # See https://github.com/FreeCAD/FreeCAD/issues/18658 and related OCCT
        #   ticket
        if sweep.Shape.isNull() or not sweep.Shape.isValid():
            if is_wall_straight:
                _log(f"Sweep's shape is invalid, using ruled surface instead…")
                App.ActiveDocument.removeObject(sweep.Label)
                compound_solid, base_object = self._make_compound(section_start, section_end, spine)
                wall = Arch.makeWall(compound_solid)
            else:
                _wrn(f"Sweep's shape is invalid, but mitigation is not available!")
                wall = Arch.makeWall(sweep)
        else:
            wall = Arch.makeWall(sweep)

        if debug_geometry:
            wall.ViewObject.DisplayMode = 'Wireframe'
            wall.ViewObject.DrawStyle = 'Dotted'
            wall.ViewObject.LineColor = ORANGE
            wall.ViewObject.LineWidth = 2
            self._debug_point(spine.Start, f"{wall.Name}-start")

        reference_face = self._get_reference_face(wall, is_wall_straight)
        if reference_face:
            self.setp(wall, "App::PropertyLink", "ReferenceFace", "The Reference Part.Wire", reference_face)
            floor.getObject(floor.ReferenceFacesGroupName).addObject(reference_face)
        else:
            _err(f"Failed to get the reference face for wall {wall.Name}. Slab might fail!")

        # Keep track of base objects. Used to decorate walls
        self.importer.set_property(wall, "App::PropertyLinkList", "BaseObjects", "The different base objects whose sweep failed. Kept for compatibility reasons", [section_start, section_end, spine])

        # TODO: Width is incorrect when joining walls
        wall.setExpression('Length', f'{spine.Name}.Length')
        wall.setExpression('Width', f'({section_start.Name}.Length + {section_end.Name}.Length) / 2')
        wall.setExpression('Height', f'({section_start.Name}.Height + {section_end.Name}.Height) / 2')

        return wall, base_object

    def _make_sweep(self, section_start, section_end, spine):
        """Creates a Part::Sweep from sections and a spine.

        Args:
            section_start (Rectangle): the first section of the Sweep
            section_end (Rectangle): the last section of the Sweep
            spine (Line): the path of the Sweep

        Returns:
            Part::Sweep: the Part::Sweep
        """
        sweep = App.ActiveDocument.addObject('Part::Sweep', "WallShape")
        sweep.Sections = [section_start, section_end]
        sweep.Spine = spine
        sweep.Solid = True
        sweep.Frenet = False
        section_start.Visibility = False
        section_end.Visibility = False
        spine.Visibility = False
        sweep.recompute(True)
        return sweep

    def _make_compound(self, section_start, section_end, spine):
        """Creates a compound from sections

        This is used as a mitigation for a criss-crossed Part::Sweep.

        Args:
            section_start (Rectangle): the first section of the Sweep
            section_end (Rectangle): the last section of the Sweep
            spine (Line): not really used...

        Returns:
            Compound: the compound
        """
        ruled_surface = App.ActiveDocument.addObject('Part::RuledSurface')
        ruled_surface.Curve1 = section_start
        ruled_surface.Curve2 = section_end
        ruled_surface.recompute()
        compound = App.activeDocument().addObject("Part::Compound")
        compound.Links = [ruled_surface, section_start, section_end, spine]
        compound.recompute()

        compound_solid = App.ActiveDocument.addObject("Part::Feature", "WallShape")
        compound_solid.Shape = Part.Solid(Part.Shell(compound.Shape.Faces))

        return compound_solid, compound

    def _get_wall_details(self, floor, elm):
        """Returns the relevant element for the given wall.

        Args:
            floor (Slab): the Slab the wall belongs to
            elm (Element): the wall being imported

        Returns:
            Vector: the wall's starting point
            vector: the wall's ending point
            float: the thickness
            float: the wall's height at the starting point
            float: the wall's height at the ending point
            float: the wall's arc in degrees
        """
        if elm is None:
            return None
        x_start = float(elm.get('xStart'))
        y_start = float(elm.get('yStart'))
        x_end = float(elm.get('xEnd'))
        y_end = float(elm.get('yEnd'))
        z = dim_fc2sh(floor.Placement.Base.z)

        thickness = dim_sh2fc(elm.get('thickness'))
        arc_extent = ang_sh2fc(elm.get('arcExtent', 0))
        height_start = dim_sh2fc(elm.get('height', dim_fc2sh(floor.Height)))
        height_end = dim_sh2fc(elm.get('heightAtEnd', dim_fc2sh(height_start)))

        start = coord_sh2fc(App.Vector(x_start, y_start, z))
        end = coord_sh2fc(App.Vector(x_end, y_end, z))

        return (start, end, thickness, height_start, height_end, arc_extent)

    def _create_straight_segment(self, wall_details, prev_wall_details, next_wall_details):
        """Returns the sections and spine for a straight wall.

        Args:
            wall_details (tuple): the wall details for the wall being imported
            prev_wall_details (tuple): the details for the previous sibling
            next_wall_details (tuple): the details for the next sibling

        Returns:
            Rectangle, Rectangle, spine: both section and the line for the wall
        """
        (start, end, _, _, _, _) = wall_details

        a1, a2, _ = self._get_normal_angles(wall_details)

        section_start = self._get_section(wall_details, True, prev_wall_details, a1, a2)
        section_end = self._get_section(wall_details, False, next_wall_details, a1, a2)

        spine = Draft.makeLine(start, end)

        return section_start, section_end, spine

    def _create_curved_segment(self, wall_details, prev_wall_details, next_wall_details):
        """Returns the sections and spine for a curved wall.

        Args:
            wall_details (tuple): the wall details for the wall being imported
            prev_wall_details (tuple): the details for the previous sibling
            next_wall_details (tuple): the details for the next sibling

        Returns:
            Rectangle, Rectangle, spine: both section and the arc for the wall
        # """
        (start, end, _, _, _, _) = wall_details

        a1, a2, (invert_angle, center, radius) = self._get_normal_angles(wall_details)

        section_start = self._get_section(wall_details, True, prev_wall_details, a1, a2)
        section_end = self._get_section(wall_details, False, next_wall_details, a1, a2)

        if self.importer.preferences["DEBUG_GEOMETRY"]:
            self._debug_vector(start-center, "start-center", GREEN, center)
            self._debug_vector(end-center, "end-center", BLUE, center)

        placement = App.Placement(center, App.Rotation())
        # BEWARE: makeCircle always draws counter-clockwise (i.e. in positive
        # direction in xYz coordinate system). We therefore need to invert
        # the start and end angle (as in SweetHome the wall is drawn in
        # clockwise fashion).
        length = abs(radius * math.radians(a2 - a1))
        if invert_angle:
            spine = Draft.makeCircle(radius, placement, False, a1, a2)
        else:
            spine = Draft.makeCircle(radius, placement, False, a2, a1)

        # The Length property is used in the Wall to calculate volume, etc...
        # Since make Circle does not calculate this Length I calculate it here...
        self.importer.set_property(spine, "App::PropertyFloat", "Length", "The length of the Arc", length, group="Draft")
        # The Start and End property are used in the Wall to determine Facebinders
        # characteristics...
        self.importer.set_property(spine, "App::PropertyVector", "Start", "The start point of the Arc", start, group="Draft")
        self.importer.set_property(spine, "App::PropertyVector", "End", "The end point of the Arc", end, group="Draft")

        return section_start, section_end, spine

    def _get_section(self, wall_details, at_start, sibling_details, a1, a2):
        """Returns a rectangular section at the specified coordinate.

        Returns a Rectangle that is then used as a section in the Part::Sweep
        used to construct a wall. Depending whether the wall should be joined
        with its siblings, the rectangle is either created and rotated around
        the endpoint of the line that will be used as the spline of the sweep
        or it is calculated as the intersection profile of the 2 walls.

        Args:
            wall_details (tuple): The details of the wall
            at_start (bool): indicate whether the section is for the start
                point or the end point of the wall.
            sibling_details (tuple): The details of the sibling wall

        Returns:
            Rectangle: the section properly positioned
        """
        debug_geometry = self.importer.preferences["DEBUG_GEOMETRY"]
        if self.importer.preferences["JOIN_ARCH_WALL"] and sibling_details:
            # In case the walls are to be joined we determine the intersection
            # of both wall which depends on their respective thickness.
            # Calculate the left and right side of each wall
            (start, end, thickness, height_start, height_end, _) = wall_details
            (s_start, s_end, s_thickness, _, _, _) = sibling_details

            lside, rside = self._get_sides(start, end, thickness)
            s_lside, s_rside = self._get_sides(s_start, s_end, s_thickness)
            i_start, i_end = self._get_intersection_edge(lside, rside, s_lside, s_rside)

            height = height_start if at_start else height_end
            i_start_z = i_start + App.Vector(0, 0, height)
            i_end_z = i_end + App.Vector(0, 0, height)

            if debug_geometry:
                _log(f"Joining wall {self._pv(end-start)}@{self._pv(start)} and wall {self._pv(s_end-s_start)}@{self._pv(s_start)}")
                _log(f"    wall: {self._pe(lside)},{self._pe(rside)}")
                _log(f" sibling: {self._pe(s_lside)},{self._pe(s_rside)}")
                _log(f"intersec: {self._pv(i_start)},{self._pv(i_end)}")
            section = Draft.makeRectangle([i_start, i_end, i_end_z, i_start_z], face=True)
            if debug_geometry:
                _log(f"section: {section}")
        else:
            (start, end, thickness, height_start, height_end, _) = wall_details
            height = height_start if at_start else height_end
            center = start if at_start else end
            z_rotation = a1 if at_start else a2
            section = Draft.makeRectangle(thickness, height, face=True)
            Draft.move([section], App.Vector(-thickness/2, 0, 0))
            Draft.rotate([section], 90, ORIGIN, X_NORM)
            Draft.rotate([section], z_rotation, ORIGIN, Z_NORM)
            Draft.move([section], center)

        section.recompute()
        if debug_geometry:
            _color_section(section)

        return section

    def _get_intersection_edge(self, lside, rside, sibling_lside, sibling_rside):
        """Returns the intersection edge of the 4 input edges.

        Args:
            lside (Edge): the wall left handside
            rside (Edge): the wall right handside
            sibling_lside (Edge): the sibling wall left handside
            sibling_rside (Edge): the sibling wall right handside

        Returns:
            Edge: the Edge starting at the left handsides intersection and the
                the right handsides intersection.
        """
        points = DraftGeomUtils.findIntersection(lside, sibling_lside, True, True)
        left = points[0] if len(points) else lside.Vertexes[0].Point
        points = DraftGeomUtils.findIntersection(rside, sibling_rside, True, True)
        right = points[0] if len(points) else rside.Vertexes[0].Point
        edge = DraftGeomUtils.edg(left, right)
        return edge.Vertexes[1].Point, edge.Vertexes[0].Point

    def _get_normal_angles(self, wall_details):
        """Return the angles of the normal at the endpoints of the wall.

        This method returns the normal angle of the sections that constitute
        the wall sweep. These angles can then be used to create the
        corresponding sections. Depending on whether the wall section is
        straight or curved, the section will be calculated slightly
        differently.

        Args:
            wall_details (tuple): The details of the wall

        Returns:
            float: the angle of the normal at the starting point
            float: the angle of the normal at the ending point
            bool: the angle of the normal at the ending point
            Vector: the center of the circle for a curved wall section
            float: the radius of said circle
        """
        (start, end, _, _, _, arc_extent) = wall_details

        angle_start = angle_end = 0
        invert_angle = False
        center = radius = None
        if arc_extent == 0:
            # Straight Wall...
            angle_start = angle_end = 90 - norm_deg_ang(DraftVecUtils.angle(end-start, X_NORM))
        else:
            # Calculate the circle that pases through the center of both rectangle
            #   and has the correct angle between p1 and p2
            chord = DraftVecUtils.dist(start, end)
            radius = abs(chord / (2*math.sin(arc_extent/2)))

            circles = DraftGeomUtils.circleFrom2PointsRadius(start, end, radius)
            # We take the center that preserve the arc_extent orientation (in FC
            #   coordinate). The orientation is calculated from start to end
            center = circles[0].Center
            angle = norm_deg_ang(DraftVecUtils.angle(start-center, end-center, Z_NORM))
            if self.importer.preferences["DEBUG_GEOMETRY"]:
                _msg(f"arc_extent={norm_deg_ang(arc_extent)}, angle={angle}")
            if norm_deg_ang(arc_extent) != angle:
                invert_angle = True
                center = circles[1].Center

            # radius1 and radius2 are the vector from center to start and end respectively
            radius1 = start - center
            radius2 = end - center

            angle_start = norm_deg_ang(DraftVecUtils.angle(X_NORM, radius1, Z_NORM))
            angle_end = norm_deg_ang(DraftVecUtils.angle(X_NORM, radius2, Z_NORM))

        return angle_start, angle_end, (invert_angle, center, radius)

    def _get_sides(self, start, end, thickness):
        """Return 2 edges corresponding to the left and right side of the wall.

        Args:
            start (Vector): the wall's starting point
            end (Vector): the wall's ending point
            thickness (float): the wall's thickness

        Returns:
            Edge: the left handside edge of the wall
            Edge: the right handside edge of the wall
        """
        normal = self._get_normal(start, end, start+Z_NORM)
        loffset = DraftVecUtils.scale(-normal, thickness/2)
        roffset = DraftVecUtils.scale(normal, thickness/2)
        edge = DraftGeomUtils.edg(start, end)
        lside = DraftGeomUtils.offset(edge, loffset)
        rside = DraftGeomUtils.offset(edge, roffset)
        if self.importer.preferences["DEBUG_GEOMETRY"]:
            _log(f"_get_sides(): wall {self._pv(end-start)}@{self._pv(start)} => normal={self._pv(normal)}, lside={self._pe(lside)}, rside={self._pe(rside)}")
        return lside, rside

    def _get_normal(self, a, b, c):
        """Return the normal of a plane defined by 3 points.

        NOTE: the order of your point is important as the coordinate
            will go from a to b to c

        Args:
            a (Vector): the first point
            b (Vector): the second point
            c (Vector): the third point

        Returns:
            Vector: the normalized vector of the plane's normal
        """
        return (b - a).cross(c - a).normalize()

    def _get_reference_face(self, wall, is_wall_straight):
        """Returns the reference face for a wall.

        There are some strange situation when the bottom face is self-intersecting.
        This will result in an invalid extrude and will therefore fail the slab
        creation. This solved by creating a convex hull. Note that this mitigation
        is only used for straight walls. Curved walls do not seem to be affected
        by the problem.

        Args:
            wall (Arch.Wall): the wall for which to create the reference face

        Returns:
            Part.Wire: the wire for the reference face
        """
        # Extract the reference face for later use (when creating the slab)
        bottom_faces = list(filter(lambda f: Z_NORM.isEqual(-f.normalAt(0,0),1e-6), wall.Base.Shape.Faces))

        if len(bottom_faces) == 0:
            return None

        if len(bottom_faces) > 1:
            _wrn(f"Base object for wall {wall.Name} has several bottom facing reference faces! Defaulting to 1st one.")

        face = bottom_faces.pop(0)

        if is_wall_straight:
            # In order to make sure that the edges are not self-intersecting
            # create a convex hull and use these points instead. Maybe
            # overkill for a 4 point wall, however not sure how to invert
            # edges.
            points = list(map(lambda v: v.Point, face.Vertexes))
            new_points = convex_hull(points)
            reference_face = Draft.make_wire(new_points, closed=True, face=True, support=None)
        else:
            reference_face = App.ActiveDocument.addObject("Part::Feature", "Face")
            reference_face.Shape = face

        reference_face.Label = f"{wall.Name}-reference"
        reference_face.Visibility = False
        reference_face.recompute()
        return reference_face

    def post_process(self, obj):
        if self.importer.preferences["DECORATE_SURFACES"]:
            floor = App.ActiveDocument.getObject(obj.ReferenceFloorName)

            (left_face_name, left_face, right_face_name, right_face) = self.get_faces(obj)

            self._create_facebinders(floor, obj, left_face_name, right_face_name)

            self._create_baseboards(floor, obj, left_face, right_face)

    def _create_facebinders(self, floor, wall, left_face_name, right_face_name):
        """Set the wall's colors taken from `elm`.

        Creates 2 FaceBinders (left and right) and sets the corresponding
        color and the shininess of the wall.

        Args:
            floor (Arch::Level): the level the wall belongs to. Used to group
                the resulting Facebinders
            wall (Arch::Wall): the wall to paint
            elm (Element): the xml element for the wall to be imported
            left_face_name (str): the name of the left face suitable for selecting
            right_face_name (str): the name of the right face suitable for selecting
        """
        # The top color is the color of the "mass" of the wall
        top_color = wall.topColor
        set_color_and_transparency(wall, top_color)
        self._create_facebinder(floor, wall,left_face_name,  "left")
        self._create_facebinder(floor, wall, right_face_name,  "right")

    def _create_facebinder(self, floor, wall, face_name, side):
        if face_name:
            facebinder_id = f"{wall.id}-{side}-facebinder"
            facebinder = None
            if self.importer.preferences["MERGE"]:
                facebinder = self.get_fc_object(facebinder_id, 'facebinder')

            if not facebinder:
                facebinder = Draft.make_facebinder(( wall, (face_name, ) ))
                facebinder.Extrusion = 1
                facebinder.Label = wall.Label + f" {side} side finish"

            color = getattr(wall, f"{side}SideColor")
            set_color_and_transparency(facebinder, color)
            shininess = getattr(wall, f"{side}SideShininess", 0)
            set_shininess(facebinder, shininess)
            self.setp(facebinder, "App::PropertyString", "shType", "The element type", 'facebinder')
            self.setp(facebinder, "App::PropertyString", "id", "The element's id", facebinder_id)
            self.setp(facebinder, "App::PropertyString", "ReferenceWallName", "The element's wall Name", wall.Name)

            floor.getObject(floor.DecorationWallsGroupName).addObject(facebinder)
        else:
            _wrn(f"Failed to determine {side} face for wall {wall.Label}!")

    def _create_baseboards(self, floor, wall, left_face, right_face):
        """Creates and returns a Part::Extrusion from the imported_baseboard object

        Args:
            floor (Slab): the Slab the wall belongs to
            wall (Wall): the Arch wall
            elm (Element): the wall being imported (with child baseboards)
            left_face (Part.Face): the left hand side of the wall
            right_face (Part.Face): the right hand side of the wall

        Returns:
            Part::Extrusion: the newly created object
        """
        for side in ["leftSideBaseboard", "rightSideBaseboard"]:
            if hasattr(wall, f"{side}Height"):
                face = left_face if side == "leftSideBaseboard" else right_face
                if not face:
                    _err(f"Weird: Invalid {side} face for wall {wall.Label}. Skipping baseboard creation")
                    continue
                self._create_baseboard(floor, wall, side, face)

    def _create_baseboard(self, floor, wall, side, face):

        baseboard_width = getattr(wall, f"{side}Thickness").Value
        baseboard_height = getattr(wall, f"{side}Height").Value

        # Once I have the face, I get the lowest edge.
        lowest_z = float('inf')
        bottom_edge = None

        for edge in face.Edges:
            if edge and edge.CenterOfGravity and edge.CenterOfGravity.z < lowest_z:
                lowest_z = edge.CenterOfGravity.z
                bottom_edge = edge

        p_normal = face.normalAt(bottom_edge.CenterOfGravity.x, bottom_edge.CenterOfGravity.y)
        p_normal.z = 0
        offset_vector = p_normal.normalize().multiply(baseboard_width)
        offset_bottom_edge = bottom_edge.translated(offset_vector)

        if self.importer.preferences["DEBUG_GEOMETRY"]:
            _log(f"Creating {side} for {wall.Label} from edge {self._pe(bottom_edge, True)} to {self._pe(offset_bottom_edge, True)} (normal={self._pv(p_normal, True, 4)})")

        edge0 = bottom_edge.copy()
        edge1 = Part.makeLine(bottom_edge.Vertexes[1].Point, offset_bottom_edge.Vertexes[1].Point)
        edge2 = offset_bottom_edge
        edge3 = Part.makeLine(offset_bottom_edge.Vertexes[0].Point, bottom_edge.Vertexes[0].Point)

        # make sure all edges are coplanar...
        ref_z = bottom_edge.CenterOfGravity.z
        for edge in [edge0, edge1, edge2, edge3]:
            edge.Vertexes[0].Point.z = edge.Vertexes[1].Point.z = ref_z

        baseboard_id = f"{wall.id} {side}"
        baseboard = None
        if self.importer.preferences["MERGE"]:
            baseboard = self.get_fc_object(baseboard_id, 'baseboard')

        if not baseboard:
            base = App.ActiveDocument.addObject("Part::Feature", f"{wall.Label} {side} base")
            base.Shape = Part.makeFace([ Part.Wire([edge0, edge1, edge2, edge3]) ])
            base.Visibility = False
            baseboard = App.ActiveDocument.addObject('Part::Extrusion', f"{wall.Label} {side}")
            baseboard.Base = base

        baseboard.DirMode = "Custom"
        baseboard.Dir = Z_NORM
        baseboard.DirLink = None
        baseboard.LengthFwd = baseboard_height
        baseboard.LengthRev = 0
        baseboard.Solid = True
        baseboard.Reversed = False
        baseboard.Symmetric = False
        baseboard.TaperAngle = 0
        baseboard.TaperAngleRev = 0

        set_color_and_transparency(baseboard, getattr(wall, f"{side}Color"))

        self.setp(baseboard, "App::PropertyString", "shType", "The element type", 'baseboard')
        self.setp(baseboard, "App::PropertyString", "id", "The element's id", baseboard_id)
        self.setp(baseboard, "App::PropertyString", "ReferenceWallName", "The element's wall Name", wall.Name)

        baseboard.recompute(True)
        floor.getObject(floor.DecorationBaseboardsGroupName).addObject(baseboard)


class BaseFurnitureHandler(BaseHandler):
    """The base class for importing different class of furnitures."""

    def __init__(self, importer: SH3DImporter):
        super().__init__(importer)

    def set_furniture_common_properties(self, obj, elm):
        self.setp(obj, "App::PropertyString", "id", "The furniture's id", elm)
        self.setp(obj, "App::PropertyString", "name", "The furniture's name", elm)
        self.setp(obj, "App::PropertyFloat", "angle", "The angle of the furniture", elm)
        self.setp(obj, "App::PropertyBool", "visible", "Whether the object is visible", elm)
        self.setp(obj, "App::PropertyBool", "movable", "Whether the object is movable", elm)
        self.setp(obj, "App::PropertyString", "description", "The object's description", elm)
        self.setp(obj, "App::PropertyString", "information", "The object's information", elm)
        self.setp(obj, "App::PropertyString", "license", "The object's license", elm)
        self.setp(obj, "App::PropertyString", "creator", "The object's creator", elm)
        self.setp(obj, "App::PropertyBool", "modelMirrored", "Whether the object is mirrored", bool(elm.get('modelMirrored', False)))
        self.setp(obj, "App::PropertyBool", "nameVisible", "Whether the object's name is visible", bool(elm.get('nameVisible', False)))
        self.setp(obj, "App::PropertyFloat", "nameAngle", "The object's name angle", elm)
        self.setp(obj, "App::PropertyFloat", "nameXOffset", "The object's name X offset", elm)
        self.setp(obj, "App::PropertyFloat", "nameYOffset", "The object's name Y offset", elm)
        self.setp(obj, "App::PropertyFloat", "price", "The object's price", elm)

    def set_piece_of_furniture_common_properties(self, obj, elm):
        self.setp(obj, "App::PropertyString", "level", "The furniture's level", elm)
        self.setp(obj, "App::PropertyString", "catalogId", "The furniture's catalog id", elm)
        self.setp(obj, "App::PropertyFloat", "dropOnTopElevation", "", elm)
        self.setp(obj, "App::PropertyString", "model", "The object's mesh file", elm)
        self.setp(obj, "App::PropertyString", "icon", "The object's icon", elm)
        self.setp(obj, "App::PropertyString", "planIcon", "The object's icon for the plan view", elm)
        self.setp(obj, "App::PropertyString", "modelRotation", "The object's model rotation", elm)
        self.setp(obj, "App::PropertyString", "modelCenteredAtOrigin", "The object's center", elm)
        self.setp(obj, "App::PropertyBool", "backFaceShown", "Whether the object's back face is shown", elm)
        self.setp(obj, "App::PropertyString", "modelFlags", "The object's flags", elm)
        self.setp(obj, "App::PropertyFloat", "modelSize", "The object's size", elm)
        self.setp(obj, "App::PropertyBool", "doorOrWindow", "Whether the object is a door or Window", bool(elm.get('doorOrWindow', False)))
        self.setp(obj, "App::PropertyBool", "resizable", "Whether the object is resizable", elm)
        self.setp(obj, "App::PropertyBool", "deformable", "Whether the object is deformable", elm)
        self.setp(obj, "App::PropertyBool", "texturable", "Whether the object is texturable", elm)
        self.setp(obj, "App::PropertyString", "staircaseCutOutShape", "", elm)
        self.setp(obj, "App::PropertyPercent", "shininess", "The object's shininess", percent_sh2fc(elm.get('shininess', 0)))
        self.setp(obj, "App::PropertyFloat", "valueAddedTaxPercentage", "The object's VAT percentage", elm)
        self.setp(obj, "App::PropertyString", "currency", "The object's price currency", str(elm.get('currency', 'EUR')))

    def set_piece_of_furniture_horizontal_rotation_properties(self, obj, elm):
        self.setp(obj, "App::PropertyBool", "horizontallyRotatable", "Whether the object horizontally rotatable", elm)
        self.setp(obj, "App::PropertyFloat", "pitch", "The object's pitch", elm)
        self.setp(obj, "App::PropertyFloat", "roll", "The object's roll", elm)
        self.setp(obj, "App::PropertyFloat", "widthInPlan", "The object's width in the plan view", elm)
        self.setp(obj, "App::PropertyFloat", "depthInPlan", "The object's depth in the plan view", elm)
        self.setp(obj, "App::PropertyFloat", "heightInPlan", "The object's height in the plan view", elm)

    def _get_mesh(self, elm):
        model = elm.get('model')
        if model not in self.importer.zip.namelist():
            raise ValueError(f"Invalid SweetHome3D file: missing model {model} for furniture {elm.get('id')}")
        model_path_obj = None
        try:
            # Since mesh.read(model_data) does not work on BytesIO extract it first
            tmp_dir = App.ActiveDocument.TransientDir
            if os.path.isdir(os.path.join(tmp_dir, model)):
                tmp_dir = os.path.join(tmp_dir, str(uuid.uuid4()))
            model_path = self.importer.zip.extract(member=model, path=tmp_dir)
            model_path_obj = model_path+".obj"
            os.rename(model_path, model_path_obj)
            mesh = Mesh.Mesh()
            mesh.read(model_path_obj)
        finally:
            os.remove(model_path_obj)
        return mesh


class DoorOrWindowHandler(BaseFurnitureHandler):
    """A helper class to import a SH3D `<doorOrWindow>` object."""

    def __init__(self, importer: SH3DImporter):
        super().__init__(importer)

    def process(self, parent, i, elm):
        """Creates and returns a Arch::Door from the imported_door object

        Args:
            i (int): the ordinal of the imported element
            elm (Element): the xml element
        """
        door_id = f"{elm.get('id', elm.get('name'))}-{i}"
        level_id = elm.get('level', None)
        floor = self.get_floor(level_id)
        assert floor != None, f"Missing floor '{level_id}' for <doorOrWindow> '{door_id}'…"


        feature = None
        if self.importer.preferences["MERGE"]:
            feature = self.get_fc_object(door_id, 'doorOrWindow')

        if not feature:
            feature = self._create_door(floor, elm)

        if not feature:
            return

        self._set_properties(feature, elm)
        self.set_furniture_common_properties(feature, elm)
        self.set_piece_of_furniture_common_properties(feature, elm)
        self.setp(feature, "App::PropertyString", "id", "The furniture's id", door_id)

    def _set_properties(self, obj, elm):
        self.setp(obj, "App::PropertyString", "shType", "The element type", 'doorOrWindow')
        self.setp(obj, "App::PropertyFloat", "wallThickness", "", dim_sh2fc(elm.get('wallThickness', 1)))
        self.setp(obj, "App::PropertyFloat", "wallDistance", "", dim_sh2fc(elm.get('wallDistance', 0)))
        self.setp(obj, "App::PropertyFloat", "wallWidth", "", dim_sh2fc(elm.get('wallWidth', 1)))
        self.setp(obj, "App::PropertyFloat", "wallLeft", "", dim_sh2fc(elm.get('wallLeft', 0)))
        self.setp(obj, "App::PropertyFloat", "wallHeight", "", dim_sh2fc(elm.get('wallHeight', 1)))
        self.setp(obj, "App::PropertyFloat", "wallTop", "", dim_sh2fc(elm.get('wallTop', 0)))
        self.setp(obj, "App::PropertyBool", "wallCutOutOnBothSides", "", elm)
        self.setp(obj, "App::PropertyBool", "widthDepthDeformable", "", elm)
        self.setp(obj, "App::PropertyString", "cutOutShape", "", elm)
        self.setp(obj, "App::PropertyBool", "boundToWall", "", elm)

    def _create_door(self, floor, elm):
        debug_geometry = self.importer.preferences["DEBUG_GEOMETRY"]
        # The doorOrWndow in SH3D is defined with a width, depth, height.
        # Furthermore the (x.y.z) is the center point of the lower face of the
        # window. In FC the placement is defined on the face of the wall that
        # contains the windows and it references the corner of said face.
        # Therefore translating the n arbitrary volume in SH3D into a face in
        # FreeCAD is rather confusing and tricky to get right.
        x_center = float(elm.get('x'))
        y_center = float(elm.get('y'))
        z_center = float(elm.get('elevation', 0))

        label_prefix = f"dow-{elm.get('id')}"

        # The absolute coordinate of the center of the doorOrWndow's lower face
        dow_abs_center = coord_sh2fc(App.Vector(x_center, y_center, z_center))
        dow_abs_center.z += floor.Placement.Base.z
        width = dim_sh2fc(elm.get('width'))
        depth = dim_sh2fc(elm.get('depth'))
        height = dim_sh2fc(elm.get('height'))
        angle = norm_deg_ang(ang_sh2fc(elm.get('angle', 0)))

        # Note that we only move on the XY plane since we assume that
        # only the right and left face will be used for supporting the
        # doorOrWndow. It might not be correct for roof windows and floor
        # windows...
        # The absolute coordinate of the corner of the doorOrWindow
        dow_abs_corner = dow_abs_center.add(App.Vector(-width/2, -depth/2, 0))

        # Create a solid representing the BoundingBox of the windows
        # to find out which walls contains the window...
        dow_bounding_box = Part.makeBox(width, depth, height, dow_abs_corner)
        dow_bounding_box = dow_bounding_box.rotate(dow_bounding_box.CenterOfGravity, Z_NORM, angle)
        if debug_geometry:
            self._debug_shape(dow_bounding_box, f"{label_prefix}-bb", BLUE)
            self._debug_point(dow_bounding_box.CenterOfGravity, f"{label_prefix}-bb-cog")

        # Indicate whether the bounding box CoG is inscribed in the wall
        is_opened = False
        wall_width = depth
        # Get all the walls hosting that doorOrWndow.
        #
        # The main wall is used to determine the projection of the
        # doorOrWindow bounding_box, and thus the placement of the
        # resulting Arch element. The main wall is the one containing
        # the CenterOfGravity of the bounding_box. Note that for opened
        # windows the CenterOfGravity might be outside any wall. In which
        # case we only take the first wall hosting the window.
        main_wall, extra_walls = self._get_containing_walls(floor, dow_bounding_box)
        if main_wall:
            wall_width = main_wall.Width.Value
        else:
            if len(extra_walls) == 0:
                _err(f"No hosting wall for doorOrWindow#{elm.get('id')}. Bailing out!")
                if debug_geometry: self._debug_shape(dow_bounding_box, f"{label_prefix}-no-hosting-wall", RED)
                return None
            # Hum probably open doorOrWndow?
            is_opened = True
            main_wall = extra_walls.pop(0)
            wall_width = main_wall.Width.Value
            if len(extra_walls) > 0:
                _wrn(f"No main hosting wall for doorOrWindow#{elm.get('id')}. Defaulting to first hosting wall#{main_wall.Label} (w/ width {wall_width})…")

        # Get the left and right face for the main_wall
        (_, wall_lface, _, wall_rface) = self.get_faces(main_wall)

        # The general process is as follow:
        # 1- Find the bounding box face whose normal is properly oriented
        #    with respect to the doorOrWindow (+90º)
        # 2- Find the wall face with the same orientation.
        # 3- Project the bounding box face onto the wall face.
        # 4- From projection extract the placement of the window.

        # Determine the bounding box face
        bb_face, bb_face_normal = self._get_bb_face(dow_bounding_box, angle, label_prefix)
        if not bb_face:
            _err(f"Weird: None of BoundingBox's faces for doorOrWindow#{elm.get('id')} has the expected angle ({angle}º). Cannot create the window.")
            if debug_geometry: self._debug_shape(dow_bounding_box, f"{label_prefix}-missing-bb-face#{main_wall.Label}", RED)
            return None
        elif debug_geometry:
          self._debug_shape(bb_face, f"{label_prefix}-bb-face", MAGENTA)

        # Determine the wall's face with the same orientation. Note that
        # if the window is ever so slightly twisted with respect to the wall
        # this will probably fail.
        # In order to get the proper wall's Face, we calculate the normal to the
        # wall's face at the bb_face CenterOfGravity. This is to avoid problems
        # with curved walls
        # First get the u,v parameter of the bb_face CoG onto each of the wall faces
        # Then get the normal at these parameter on each of the wall faces
        wall_rface_normal = wall_rface.normalAt(*(wall_rface.Surface.parameter(bb_face.CenterOfGravity)))
        wall_lface_normal = wall_lface.normalAt(*(wall_lface.Surface.parameter(bb_face.CenterOfGravity)))
        wall_face = wall_rface
        is_on_right = True
        if not self._same_dir(bb_face_normal, wall_rface_normal, 1):
            is_on_right = False
            wall_face = wall_lface
            if not self._same_dir(bb_face_normal, wall_lface_normal, 1):
                _err(f"Weird: the extracted bb_normal {self._pv(bb_face_normal, True)} does not match neither the right face normal ({self._pv(wall_rface_normal, True)}) nor the left face normal ({self._pv(wall_lface_normal, True)}) of the wall {main_wall.Label}… The doorOrWindow might be slightly skewed. Defaulting to left face.")

        # Project the bounding_box face onto the wall
        projected_face = wall_face.makeParallelProjection(bb_face.OuterWire, bb_face_normal)
        if debug_geometry:
            self._debug_shape(wall_face, f"{label_prefix}-bb-projected-onto#{main_wall.Label}", MAGENTA)
            self._debug_shape(projected_face, f"{label_prefix}-bb-projection#{main_wall.Label}", RED)

        # Determine the base vertex that I later use for the doorOrWindow
        # placement
        base_vertex = self._get_base_vertex(main_wall, is_on_right, projected_face)

        pl = App.Placement(
          base_vertex,                # move
          App.Rotation(angle, 0, 90), # Yaw, pitch, roll
          ORIGIN                      # rotation@point
        )
        if debug_geometry: self._debug_point(pl.Base, f"{label_prefix}-pl-base", MAGENTA)

        # Then prepare the windows characteristics
        # NOTE: the windows are not imported as meshes, but we use a simple
        #   correspondence between a catalog ID and a specific window preset from
        #   the parts library. Only using Opening / Fixed / Simple Door
        catalog_id = elm.get('catalogId')
        (windowtype, ifc_type) = DOOR_MODELS.get(catalog_id, (None, None))
        if not windowtype:
            _wrn(f"Unknown catalogId {catalog_id} for element {elm.get('id')}. Defaulting to 'Simple Door'")
            (windowtype, ifc_type) = ('Simple door', 'Door')

        # See the https://wiki.freecad.org/Arch_Window for details about these values
        # NOTE: These are simple heuristic to get reasonable windows
        h1 = min(50, height*.025)     # frame is 2.5% of whole height...
        h2 = h1                       # panel's frame is the same as frame
        h3 = 0
        w1 = wall_width               # frame is 100% of wall width...
        w2 = min(20.0, wall_width*.2) # panel is 20% of wall width
        o1 = (wall_width-w1)/2        # frame is centered
        o2 = (wall_width-w2)/2        # panel is centered
        window = Arch.makeWindowPreset(windowtype, width, height, h1, h2, h3, w1, w2, o1, o2, pl)
        window.Label = elm.get('name')
        window.IfcType = ifc_type
        if is_opened: window.Opening = 30

        # Adjust symbol plan, Sweet Home has the opening in the opposite side by default
        window.ViewObject.Proxy.invertOpening()
        mirrored = bool(elm.get('modelMirrored', False))
        if mirrored:
            window.ViewObject.Proxy.invertHinge()

        # Finally make sure all the walls are properly cut by the doorOrWndow.
        window.Hosts = [main_wall, *extra_walls] if main_wall else extra_walls
        return window

    def _get_containing_walls(self, floor, dow_bounding_box):
        """Returns the wall(s) and slab(s) intersecting with the doorOrWindow
        bounding_box.

        The main wall is the one that contains the doorOrWndow bounding_box
        CenterOfGravity. Note that this will not work for open doorOrWindow
        (i.e.whose bounding_box is a lot greater than the containing wall).
        The _create_door, has a mitigation process for that case.

        The main_wall is used to get the face on which to project the
        doorOrWindows bounding_box, and from there the placement of the
        element on the wall's face.

        The general process is as follow:
        - find out whether the doorOrWindow span several floors, if so
          add all the walls (and slab) for that floor to the list of elements
          to check.
        - once the list of elements to check is complete we check if the
          doorOrWindow bounding_box has a volume in common with the wall.

        Args:
            floor (Arch.Level): the level the solid must belongs to
            dow_bounding_box (Part.Solid): the solid to test against each wall's
                bounding box

        Returns:
            tuple(Arch::Wall, list(Arch::Wall)): a tuple of the main wall (if
              any could be found and a list of any other Arch element that
              might be host of that
        """
        relevant_walls = [*self.importer.get_walls(floor)]
        # First find out which floor the window might be have an impact on.
        solid_zmin = dow_bounding_box.BoundBox.ZMin
        solid_zmax = dow_bounding_box.BoundBox.ZMax
        if solid_zmin < floor.Placement.Base.z or solid_zmax > (floor.Placement.Base.z + floor.Height.Value):
            # determine the impacted floors
            for other_floor in self.importer.get_all_floors():
                if other_floor.id == floor.id:
                    continue
                floor_zmin = other_floor.Placement.Base.z
                floor_zmax = other_floor.Placement.Base.z + other_floor.Height.Value
                if (floor_zmin < solid_zmin and solid_zmin < floor_zmax) or (
                    floor_zmin < solid_zmax and solid_zmax < floor_zmax) or (
                    solid_zmin < floor_zmin and floor_zmax < solid_zmax):
                    # Add floor and slabs
                    relevant_walls.extend(self.importer.get_walls(other_floor))
                    if other_floor.ReferenceSlabName:
                        relevant_walls.append(App.ActiveDocument.getObject(other_floor.ReferenceSlabName))
        main_wall = None
        host_walls = []
        # Taking the CoG projection on the lower face.
        solid_cog = dow_bounding_box.CenterOfGravity
        solid_cog.z = solid_zmin
        for wall in relevant_walls:
            if wall.Shape.isNull():
                continue
            if wall.Shape.isInside(solid_cog, 1, True):
                main_wall = wall
                continue
            if dow_bounding_box.common(wall.Shape).Volume > 0:
                host_walls.append(wall)
        return main_wall, host_walls

    def _get_bb_face(self, dow_bounding_box, angle, label_prefix=None):
        """Returns the bounding box face with the correct normal.

        Returns the bounding box face whose normal has the same orientation
        as the window itself. Note that we round and modulo 360 to avoid
        problems.

        Args:
            dow_bounding_box (Part.Solid): The window bounding box
            angle (float): the window angle in degree

        Returns:
            Part.Face: the correct face or None
        """
        debug_geometry = self.importer.preferences["DEBUG_GEOMETRY"]
        # Note that the 'angle' refers to the angle of the face, not its normal.
        # we therefore add a '+90º' in SH3D coordinate (i.e. -90º in FC
        # coordinate).
        # XXX: Can it be speed up by assuming that left and right are always
        #   Face2 and Face4???
        angle = (angle - 90) % 360 # make sure positive ccw angle
        for i, face in enumerate(dow_bounding_box.Faces):
            face_normal = face.normalAt(0,0) # The face is flat. can use u = v = 0
            normal_angle = norm_deg_ang(DraftVecUtils.angle(X_NORM, face_normal))
            if debug_geometry: _msg(f"#{i}/{label_prefix} {normal_angle}º <=> {angle}º")
            if normal_angle == angle:
                if debug_geometry: _msg(f"Found bb#{i}/{label_prefix} (@{normal_angle}º)")
                return face, face_normal
        return None, None

    def _same_dir(self, v1, v2, tol=1e-6):
        return (v1.normalize() - v2.normalize()).Length < tol

    def _get_base_vertex(self, wall, is_on_right: bool, projected_face):
        """Return the base vertex used to place a doorOrWindow.

        Returns the vertex of the projected_face that serves as the
        base for the Placement when creating the doorOrWindow. It is
        the lowest vertex and closest to the wall reference point.
        The wall reference point depends on whether we are on the
        right or left side of the wall.

        Args:
            wall (Arch::Wall): the wall
            is_on_right (bool): indicate whether the projected face is
                on the left or on the right
            projected_face (Part.Face): the bounding box projection
                on the wall

        Returns:
            App.Vector: the vector to be used as the base of the Placement
        """
        wall_spine = self.get_wall_spine(wall)
        wall_ref = wall_spine.Start if is_on_right else wall_spine.End
        lowest_z = round(projected_face.BoundBox.ZMin)
        lower_vertexes = list(filter(lambda v: round(v.Point.z) == lowest_z, projected_face.Vertexes))
        base_vertex = min(lower_vertexes, key=lambda v: v.Point.distanceToPoint(wall_ref))
        return base_vertex.Point


class FurnitureHandler(BaseFurnitureHandler):
    """A helper class to import a SH3D `<pieceOfFurniture>` object."""

    def __init__(self, importer: SH3DImporter):
        super().__init__(importer)

    def process(self, parent, i, elm):
        """Creates and returns a Mesh from the imported_furniture object

        Args:
            i (int): the ordinal of the imported element
            elm (Element): the xml element
        """
        furniture_id = self._get_furniture_id(i, elm)
        level_id = elm.get('level', None)
        floor = self.get_floor(level_id)
        assert floor != None, f"Missing floor '{level_id}' for <pieceOfFurniture> '{furniture_id}'…"

        furniture = None
        if self.importer.preferences["MERGE"]:
            furniture = self.get_fc_object(furniture_id, 'pieceOfFurniture')

        if not furniture:
            furniture = self._create_furniture(floor, elm)
            if not furniture:
                return

        color = elm.get('color', self.importer.preferences["DEFAULT_FURNITURE_COLOR"])
        set_color_and_transparency(furniture, color)

        furniture.ViewObject.DisplayMode = 'Flat Lines'

        self.setp(furniture, "App::PropertyString", "shType", "The element type", 'pieceOfFurniture')
        self.set_furniture_common_properties(furniture, elm)
        self.set_piece_of_furniture_common_properties(furniture, elm)
        self.set_piece_of_furniture_horizontal_rotation_properties(furniture, elm)
        self.setp(furniture, "App::PropertyString", "id", "The furniture's id", furniture_id)
        if 'FurnitureGroupName' not in floor.PropertiesList:
            group = floor.newObject("App::DocumentObjectGroup", "Furnitures")
            self.setp(floor, "App::PropertyString", "FurnitureGroupName", "The DocumentObjectGroup name for all furnitures on this floor", group.Name)

        # if self.importer.preferences["CREATE_ARCH_EQUIPMENT"]:
        #     p = feature.Shape.BoundBox.Center
        # else:
        #     p = feature.Mesh.BoundBox.Center

        # XXX: Furniture should be grouped in a space, but the Visibility
        #   setting is not propagated and therefore makes it not so user friendly.
        # space = self.get_space(floor, p)
        # if space:
        #     space.Group = space.Group + [feature]
        # else:
        #     _log(f"No space found to enclose {feature.Label}. Adding to generic group.")
        #     floor.getObject(floor.FurnitureGroupName).addObject(feature)
        floor.getObject(floor.FurnitureGroupName).addObject(furniture)

        # We add the object to the list of known object that can then
        # be referenced elsewhere in the SH3D model (i.e. lights).
        self.importer.add_fc_objects(furniture)

    def _create_furniture(self, floor, elm):
        debug_geometry = self.importer.preferences["DEBUG_GEOMETRY"]
        # REF:
        # - SweetHome3D/src/com/eteks/sweethome3d/model/HomePieceOfFurniture#readObject()
        # - SweetHome3D/src/com/eteks/sweethome3d/j3d/ModelManager#getNormalizedTransform()
        # - SweetHome3D/src/com/eteks/sweethome3d/j3d/ModelManager#getPieceOfFurnitureNormalizedModelTransformation()

        name = elm.get('name', elm.get('id', "NA"))

        # The general process is as follow:
        # - we load the mesh and center it properly.
        # - we apply the modelRotation, pitch and roll
        # - we scale
        # - we apply the yaw
        # - then we workout the Placement
        x = float(elm.get('x', 0.0))
        y = float(elm.get('y', 0.0))
        z = float(elm.get('elevation', 0.0))

        width = dim_sh2fc(elm.get('width', 0.0))
        depth = dim_sh2fc(elm.get('depth', 0.0))
        height = dim_sh2fc(elm.get('height', 0.0))

        pitch = norm_rad_ang(elm.get('pitch', 0.0))
        roll = norm_rad_ang(elm.get('roll', 0.0))
        angle = ang_sh2fc(elm.get('angle', 0.0))

        model_rotation = elm.get('modelRotation', None)
        model_mirrored = elm.get('modelMirrored', "false") == "true"
        model_centered_at_origin = elm.get('modelCenteredAtOrigin', "false") == "true"

        mesh = self._get_mesh(elm)

        if len(mesh.Points) == 0:
            # Until https://github.com/FreeCAD/FreeCAD/issues/19456 is solved...
            _wrn(f"Import of pieceOfFurniture#{name} resulted in an empty Mesh. Skipping.")
            return None

        model_bb = mesh.BoundBox
        if debug_geometry: self._debug_mesh(mesh, f"{name}-original", None, MAGENTA)

        mesh_transform = App.Matrix()

        mesh_transform.move(-model_bb.Center)
        if debug_geometry: self._debug_mesh(mesh, f"{name}-centered", mesh_transform, MAGENTA)

        # The model rotation is necessary to get the scaling right
        if model_rotation:
            rij = [ float(v) for v in model_rotation.split() ]
            rotation = App.Matrix(
                App.Vector(rij[0], rij[3], rij[6]),
                App.Vector(rij[1], rij[4], rij[7]),
                App.Vector(rij[2], rij[5], rij[8])
                )
            mesh_transform = rotation.multiply(mesh_transform)
            if debug_geometry: self._debug_mesh(mesh, f"{name}-rotated", mesh_transform, MAGENTA)

        if model_mirrored:
            mesh_transform.scale(-1, 1, 1) # Mirror along X
            if debug_geometry: self._debug_mesh(mesh, f"{name}-mirrored", mesh_transform)

        # We add an initial 90º in order for a yaw-pitch-roll-rotation free
        # model to appear properly in FC
        mesh_transform.rotateX(math.pi/2)
        if debug_geometry: self._debug_mesh(mesh, f"{name}-x90", mesh_transform)

        # The scaling is calculated using the models coordinate system.
        # We use a simple box to calculate the scale factors for each axis.
        # Note that we use the absolute value since the orientation will
        # be handled by the Placement.
        # Note that we do that before the model has had any ypr angles applied
        normalized_model = Part.makeBox(model_bb.XLength, model_bb.YLength, model_bb.ZLength)
        normalized_model = normalized_model.transformGeometry(mesh_transform)
        normilized_bb = normalized_model.BoundBox
        x_scale = width / normilized_bb.XLength
        y_scale = depth / normilized_bb.YLength
        z_scale = height / normilized_bb.ZLength

        mesh_transform.scale(x_scale, y_scale, z_scale)
        if debug_geometry:
            model_size = App.Vector(model_bb.XLength, model_bb.YLength, model_bb.ZLength)
            normalized_size = App.Vector(normilized_bb.XLength, normilized_bb.YLength, normilized_bb.ZLength)
            final_size = App.Vector(width, depth, height)
            factors = App.Vector(x_scale, y_scale, z_scale)
            _msg(f"{name}-size_model={self._pv(model_size, True, 1)} -> {self._pv(normalized_size, True, 1)} (x{self._pv(factors, True, 1)}) -> {self._pv(final_size, True, 1)}")
            self._debug_mesh(mesh, f"{name}-scaled", mesh_transform, MAGENTA)

        # At that point the mesh has the proper scale. We determine the placement.
        # In order to do that, we need to apply the different rotation (ypr) and
        # also the translation from the origin to the final point.
        if pitch != 0:
            r_pitch = App.Rotation(X_NORM, Radian=-pitch)
            mesh_transform = r_pitch.toMatrix().multiply(mesh_transform)
            if debug_geometry: self._debug_mesh(mesh, f"{name}-pitch", mesh_transform)
        elif roll != 0:
            r_roll = App.Rotation(Y_NORM, Radian=roll)
            mesh_transform = r_roll.toMatrix().multiply(mesh_transform)
            if debug_geometry: self._debug_mesh(mesh, f"{name}-roll", mesh_transform)
        if angle != 0:
            r_yaw = App.Rotation(Z_NORM, Radian=angle)
            mesh_transform = r_yaw.toMatrix().multiply(mesh_transform)
            if debug_geometry: self._debug_mesh(mesh, f"{name}-yaw", mesh_transform)

        mesh.transform(mesh_transform)

        # SH(x,y,z) refer to the projection of the CenterOfGravity on the
        # bottom face of the model bounding box
        translation = coord_sh2fc(App.Vector(x, y, z))
        if debug_geometry: self._debug_mesh(mesh, f"{name}-xyz", color=MAGENTA, placement=App.Placement(translation, NO_ROT))

        # Note that the SH coordinates represent the CenterOfGravity of the
        # lower face of the scaled model bounding box.
        translation.z += abs(mesh.BoundBox.ZMin)
        if debug_geometry: self._debug_mesh(mesh, f"{name}-+zmin", color=MAGENTA_LIGHT, placement=App.Placement(translation, NO_ROT))

        # Finally we add the placement of the floor itself.
        # XXX: strange that is not simply added when we add the object to the floor
        translation.z += floor.Placement.Base.z

        # The placement is ready. Note that the rotations have the origin
        # in the center of the bounding box of the scaled mesh.
        placement = App.Placement(translation, NO_ROT)

        # Ok, everything is ready to create the equipment
        if self.importer.preferences["CREATE_ARCH_EQUIPMENT"]:
            shape = Part.Shape()
            shape.makeShapeFromMesh(mesh.Topology, 1)
            furniture = Arch.makeEquipment(name="Furniture")
            furniture.Shape = shape
        else:
            furniture = App.ActiveDocument.addObject("Mesh::Feature", "Furniture")
            furniture.Mesh = mesh

        furniture.Placement = placement
        furniture.Label = elm.get('name')
        return furniture

    def _get_furniture_id(self, i, elm):
        return f"{elm.get('id', elm.get('name'))}-{i}"


class LightHandler(FurnitureHandler):
    """A helper class to import a SH3D `<lightSource>` object."""

    def __init__(self, importer: SH3DImporter):
        super().__init__(importer)

    def process(self, parent, i, elm):
        """_summary_

        Args:
            i (int): the ordinal of the imported element
            elm (Element): the xml element
        """
        light_id = super()._get_furniture_id(i, elm)
        level_id = elm.get('level', None)
        floor = self.get_floor(level_id)
        assert floor != None, f"Missing floor '{level_id}' for <doorOrWindow> '{light_id}'…"

        if self.importer.preferences["IMPORT_FURNITURES"]:
            super().process(parent, i, elm)
            light_apppliance = self.get_fc_object(light_id, 'pieceOfFurniture')
            assert light_apppliance != None, f"Missing <light> furniture {light_id}…"
            self.setp(light_apppliance, "App::PropertyFloat", "power", "The power of the light. In percent???",  float(elm.get('power', 0.5)))

        if self.importer.preferences["IMPORT_LIGHTS"]:
            # Import the lightSource sub-elments
            for j, sub_elm in enumerate(elm.findall('lightSource')):
                light_source = None
                light_source_id = f"{light_id}-{j}"
                if self.importer.preferences["MERGE"]:
                    light_source = self.get_fc_object(light_source_id, 'lightSource')

                if not light_source:
                    _, light_source, _ = PointLight.create()

                x = float(sub_elm.get('x'))
                y = float(sub_elm.get('y'))
                z = float(sub_elm.get('z'))
                diameter = float(sub_elm.get('diameter'))
                color = sub_elm.get('color')

                light_source.Label = elm.get('name')
                light_source.Placement.Base = coord_sh2fc(App.Vector(x, y, z))
                light_source.Radius = dim_sh2fc(diameter / 2)
                light_source.Color = hex2rgb(color)

                self.setp(light_source, "App::PropertyString", "shType", "The element type", 'lightSource')
                self.setp(light_source, "App::PropertyString", "id", "The elment's id", light_source_id)
                if self.importer.preferences["IMPORT_FURNITURES"]:
                    self.setp(light_source, "App::PropertyLink", "lightAppliance", "The light apppliance", light_apppliance)

                floor.getObject(floor.LightGroupName).addObject(light_source)


class CameraHandler(BaseHandler):
    """A helper class to import a SH3D `<observerCamera>` or `<camera>` objects."""

    def __init__(self, handler):
        super().__init__(handler)

    def process(self, parent, i, elm):
        """Creates and returns a Render Camera from the imported_camera object

        Args:
            i (int): the ordinal of the imported element
            elm (Element): the xml element

        Returns:
            object: the newly created object
        """
        x = float(elm.get('x'))
        y = float(elm.get('y'))
        z = float(elm.get('z'))
        yaw = float(elm.get('yaw'))
        pitch = float(elm.get('pitch'))

        attribute = elm.get('attribute')
        if attribute != "storedCamera":
            _log(translate("BIM", f"Type of <{elm.tag}> #{i} is not supported: '{attribute}'. Skipping!"))
            return

        camera_id = f"{elm.get('id', attribute)}-{i}"
        if self.importer.preferences["MERGE"]:
            camera = self.get_fc_object(camera_id, attribute)

        if not camera:
            _, camera, _ = Camera.create()

        # ¿How to convert fov to FocalLength?
        fieldOfView = float(elm.get('fieldOfView'))
        fieldOfView = math.degrees(fieldOfView)

        camera.Label = elm.get('name', attribute)
        camera.Placement.Base = coord_sh2fc(App.Vector(x, y, z))
        # NOTE: the coordinate system is screen like, thus roll & picth are inverted ZY'X''
        camera.Placement.Rotation.setYawPitchRoll(
            math.degrees(math.pi-yaw), 0, math.degrees(math.pi/2-pitch))
        camera.Projection = "Perspective"
        camera.AspectRatio = 1.33333333  # /home/environment/@photoAspectRatio

        self.setp(camera, "App::PropertyString", "shType", "The element type", 'camera')
        self.setp(camera, "App::PropertyString", "id", "The object ID", camera_id)
        self._set_properties(camera, elm)

        App.ActiveDocument.Cameras.addObject(camera)

    def _set_properties(self, obj, elm):
        self.setp(obj, "App::PropertyEnumeration", "attribute", "The type of camera", elm.get('attribute'), valid_values=["topCamera", "observerCamera", "storedCamera", "cameraPath"])
        self.setp(obj, "App::PropertyBool", "fixedSize", "Whether the object is fixed size", bool(elm.get('fixedSize', False)))
        self.setp(obj, "App::PropertyEnumeration", "lens", "The object's lens (PINHOLE | NORMAL | FISHEYE | SPHERICAL)", str(elm.get('lens', "PINHOLE")), valid_values=["PINHOLE", "NORMAL", "FISHEYE", "SPHERICAL"])
        self.setp(obj, "App::PropertyFloat", "yaw", "The object's yaw", elm)
        self.setp(obj, "App::PropertyFloat", "pitch", "The object's pitch", elm)
        self.setp(obj, "App::PropertyFloat", "time", "Unknown", elm)
        self.setp(obj, "App::PropertyFloat", "fieldOfView", "The object's FOV", elm)
        self.setp(obj, "App::PropertyString", "renderer", "The object's renderer", elm)


def dim_sh2fc(dimension):
    """Convert SweetHome dimension (cm) to FreeCAD dimension (mm)

    Args:
        dimension (float): The dimension in SweetHome

    Returns:
        float: the FreeCAD dimension
    """
    return float(dimension)*FACTOR


def dim_fc2sh(dimension):
    """Convert FreeCAD dimension (mm) to SweetHome dimension (cm)

    Args:
        dimension (float): The dimension in FreeCAD

    Returns:
        float: the SweetHome dimension
    """
    return float(dimension)/FACTOR


def coord_sh2fc(vector):
    """Converts SweetHome to FreeCAD coordinate

    Args:
        FreeCAD.Vector (FreeCAD.Vector): The coordinate in SweetHome

    Returns:
        FreeCAD.Vector: the FreeCAD coordinate
    """
    return App.Vector(vector.x*FACTOR, -vector.y*FACTOR, vector.z*FACTOR)


def ang_sh2fc(angle:float):
    """Convert SweetHome angle (º) to FreeCAD angle (º)

    SweetHome angles are clockwise positive while FreeCAD are anti-clockwise
    positive. Further more angle in FreeCAD are always positive between 0º and
    360º.

    Args:
        angle (float): The angle in SweetHome

    Returns:
        float: the FreeCAD angle
    """
    return norm_rad_ang(-float(angle))


def norm_deg_ang(angle:float):
    """Normalize a radian angle into a degree angle..

    Args:
        angle (float): The angle in radian

    Returns:
        float: a normalized angle
    """
    return round(math.degrees(float(angle)) % 360)


def norm_rad_ang(angle:float):
    """Normalize a radian angle into a radian angle..

    Args:
        angle (float): The angle in radian

    Returns:
        float: a normalized angle
    """
    return (float(angle) % TWO_PI + TWO_PI) % TWO_PI


def set_color_and_transparency(obj, color):
    if not App.GuiUp or not color:
        return

    view_object = obj.ViewObject
    if hasattr(view_object, "ShapeAppearance"):
        mat = view_object.ShapeAppearance[0]
        rgb_color = hex2rgb(color)
        mat.DiffuseColor = rgb_color
        mat.AmbientColor = rgb_color
        mat.SpecularColor = rgb_color
        mat.EmissiveColor = (0.0,0.0,0.0,1.0)
        obj.ViewObject.ShapeAppearance = (mat)
        return
    if hasattr(view_object, "ShapeColor"):
        view_object.ShapeColor = hex2rgb(color)
    if hasattr(view_object, "Transparency"):
        view_object.Transparency = _hex2transparency(color)


def color_fc2sh(hexcode):
    # 0xRRGGBBAA => AARRGGBB
    hex_str = hex(int(hexcode))[2:]
    return ''.join([hex_str[6:], hex_str[0:6]])


def hex2rgb(hexcode):
    # We might have transparency as the first 2 digit
    if isinstance(hexcode, list) or isinstance(hexcode, tuple):
        return hexcode
    if not isinstance(hexcode, str):
        assert False, "Invalid type when calling hex2rgb(), was expecting a list, tuple or string. Got "+str(hexcode)
    offset = 0 if len(hexcode) == 6 else 2
    return (
        int(hexcode[offset:offset+2], 16),   # Red
        int(hexcode[offset+2:offset+4], 16), # Green
        int(hexcode[offset+4:offset+6], 16)  # Blue
        )


def _hex2transparency(hexcode):
    if not isinstance(hexcode, str):
        assert False, "Invalid type when calling _hex2transparency(), was expecting a list, tuple or string. Got "+str(hexcode)
    return 100 - int(int(hexcode[0:2], 16) * 100 / 255)


def _color_section(section):
    view = section.ViewObject
    line_colors = [view.LineColor] * len(section.Shape.Edges)
    for i in range(0, len(line_colors)):
        line_colors[i] = hex2rgb(DEBUG_EDGES_COLORS[i%len(DEBUG_EDGES_COLORS)])
    view.LineColorArray = line_colors
    point_colors = [view.PointColor] * len(section.Shape.Vertexes)
    for i in range(0, len(point_colors)):
        point_colors[i] = hex2rgb(DEBUG_POINT_COLORS[i%len(DEBUG_POINT_COLORS)])
    view.PointColorArray = point_colors
    view.PointSize = 5


def set_shininess(obj, shininess):
    # TODO: it seems a shininess of 0 means the wall looses its
    # color. We'll leave it at the default setting until a later time
    if not App.GuiUp or not shininess:
        return
    if hasattr(obj.ViewObject, "ShapeAppearance"):
        obj.ViewObject.ShapeAppearance[0].Shininess = float(shininess)
        # obj.ViewObject.ShapeAppearance = mat


def percent_sh2fc(percent):
    # percent goes from 0 -> 1 in SH3d and 0 -> 100 in FC
    return int(float(percent)*100)


def cross_product(o, a, b):
    """Computes the cross product of vectors OA and OB."""
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

def convex_hull(points, tol=1e-6):
    """Return the convex hull of a series of Point

    Computes the convex hull using Andrew's monotone chain algorithm (NumPy version).

    Args:
        points (list): the list of point for which to find the convex hull

    Returns:
        list: the point forming the convex hull
    """
    default_z = points[0].z
    point_coords = np.array([[p.x, p.y] for p in points], dtype=np.float64)
    point_coords = point_coords[np.lexsort((point_coords[:, 1], point_coords[:, 0]))]  # Sort by x, then y

    def build_half_hull(sorted_points):
        hull = []
        for p in sorted_points:
            while len(hull) >= 2 and cross_product(hull[-2], hull[-1], p) <= tol:
                hull.pop()
            hull.append(tuple(p))
        return hull

    lower = build_half_hull(point_coords)
    upper = build_half_hull(point_coords[::-1])

    # Remove duplicates
    new_points = [App.Vector(p[0], p[1], default_z) for p in np.array(lower[:-1] + upper[:-1])]
    return new_points

# def _convex_hull(points):
#     """Return the convex hull of a series of Point
#
#     Args:
#         points (list): the list of point
#
#     Returns:
#         list: the point forming the convex hull
#     """
#     point_coords = np.array([[p.x, p.y] for p in points])
#     new_points = [points[i] for i in scipy.spatial.ConvexHull(point_coords).vertices]
#     return new_points[0]
