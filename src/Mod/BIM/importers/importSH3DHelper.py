# ***************************************************************************
# *   Copyright (c) 2024 Julien Masnada <rostskadat@gmail.com>              *
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
"""Helper functions that are used by SH3D importer."""
import math
import os
import re
import uuid
import xml.etree.ElementTree as ET
import zipfile

import Arch
import Draft
import DraftGeomUtils
import DraftVecUtils
import draftutils.gui_utils as gui_utils
import Mesh
import numpy
import Part
from draftutils.messages import _err, _log, _msg, _wrn
from draftutils.params import get_param_arch

import FreeCAD as App

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

try:
    from Render import Camera, PointLight
    from Render.project import Project
    RENDER_IS_AVAILABLE = True
except :
    RENDER_IS_AVAILABLE = False

# Sometimes, the Part::Sweep creates a "twisted" sweep that
#   impeeds the creation of the corresponding wall.
FIX_INVALID_SWEEP = False

# SweetHome3D is in cm while FreeCAD is in mm
FACTOR = 10
DEFAULT_WALL_WIDTH = 100
TOLERANCE = float(.1)

ORIGIN = App.Vector(0, 0, 0)
X_NORM = App.Vector(1, 0, 0)
Y_NORM = App.Vector(0, 1, 0)
Z_NORM = App.Vector(0, 0, 1)

# The Windows lookup map. This is really brittle and a better system should
# be found. Arch.WindowPresets =  ["Fixed", "Open 1-pane", "Open 2-pane",
#       "Sash 2-pane", "Sliding 2-pane", "Simple door", "Glass door",
#       "Sliding 4-pane", "Awning"]
# unzip -p all-windows.sh3d Home.xml | \
#   grep 'catalogId=' | \
#   sed -e 's/.*catalogId=//;s/ name=.*/: ("Open 2-pane","Window"),/' | sort -u
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

    'eTeks#doubleFrenchWindow126x200': ("Open 2-pane","Window"),
    'eTeks#doubleHungWindow80x122': ("Open 2-pane","Window"),
    'eTeks#doubleOutwardOpeningWindow': ("Open 2-pane","Window"),
    'eTeks#doubleWindow126x123': ("Open 2-pane","Window"),
    'eTeks#doubleWindow126x163': ("Open 2-pane","Window"),
    'eTeks#fixedTriangleWindow85x85': ("Open 2-pane","Window"),
    'eTeks#fixedWindow85x123': ("Open 2-pane","Window"),
    'eTeks#frenchWindow85x200': ("Open 2-pane","Window"),
    'eTeks#halfRoundWindow': ("Open 2-pane","Window"),
    'eTeks#roundWindow': ("Open 2-pane","Window"),
    'eTeks#sliderWindow126x200': ("Open 2-pane","Window"),
    'eTeks#window85x123': ("Open 2-pane","Window"),
    'eTeks#window85x163': ("Open 2-pane","Window"),
    'Kator Legaz#window-01': ("Open 2-pane","Window"),
    'Kator Legaz#window-08-02': ("Open 2-pane","Window"),
    'Kator Legaz#window-08': ("Open 2-pane","Window"),
    'Scopia#turn-window': ("Open 2-pane","Window"),
    'Scopia#window_2x1_medium_with_large_pane': ("Open 2-pane","Window"),
    'Scopia#window_2x1_with_sliders': ("Open 2-pane","Window"),
    'Scopia#window_2x3_arched': ("Open 2-pane","Window"),
    'Scopia#window_2x3': ("Open 2-pane","Window"),
    'Scopia#window_2x3_regular': ("Open 2-pane","Window"),
    'Scopia#window_2x4_arched': ("Open 2-pane","Window"),
    'Scopia#window_2x4': ("Open 2-pane","Window"),
    'Scopia#window_2x6': ("Open 2-pane","Window"),
    'Scopia#window_3x1': ("Open 2-pane","Window"),
    'Scopia#window_4x1': ("Open 2-pane","Window"),
    'Scopia#window_4x3_arched': ("Open 2-pane","Window"),
    'Scopia#window_4x3': ("Open 2-pane","Window"),
    'Scopia#window_4x5': ("Open 2-pane","Window"),

}

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
        self.walls = []

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
            self.progress_bar.start(f"Importing SweetHome 3D Home. Please wait ...", -1)
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
            self.progress_bar.start(f"Importing SweetHome 3D file '{self.filename}'. Please wait ...", -1)
        with zipfile.ZipFile(self.filename, 'r') as zip:
            self.zip = zip
            entries = zip.namelist()
            if "Home.xml" not in entries:
                raise ValueError(f"Invalid SweetHome3D file {self.filename}: missing Home.xml")
            self._import_home(ET.fromstring(zip.read("Home.xml")))

    def _import_home(self, home):
        doc = App.ActiveDocument
        self.total_object_count = self._get_object_count(home)
        _msg(f"Importing home '{home.get('name')}' ...")
        # Create the groups to organize the different resources together
        self._create_groups()

        # Get all the FreeCAD object in the active doc, in order to allow
        # for merge of existing object
        if self.preferences["MERGE"]:
            for object in doc.Objects:
                if hasattr(object, 'id'):
                    self.fc_objects[object.id] = object

        # Let's create the project and site for this import
        self._setup_project(home)

        # Import the <level> element if any. If none are defined
        # create a default one.
        if home.find(path='level') != None:
            self._import_elements(home, 'level')
        else:
            # Has the default floor already been created from a
            # previous import?
            if self.preferences["DEBUG"]: _log("No level defined. Using default level ...")
            self.default_floor = self.fc_objects.get('Level') if 'Level' in self.fc_objects else self._create_default_floor()
            self.add_floor(self.default_floor)

        # Importing <room> elements ...
        self._import_elements(home, 'room')

        # Importing <wall> elements ...
        self._import_elements(home, 'wall')

        self._refresh()
        if App.GuiUp and self.preferences["FIT_VIEW"]:
            Gui.SendMsgToActiveView("ViewFit")

        # Importing <doorOrWindow> elements ...
        if self.preferences["IMPORT_DOORS_AND_WINDOWS"]:
            self._import_elements(home, 'doorOrWindow')
            self._refresh()

        # Importing <pieceOfFurniture> && <furnitureGroup> elements ...
        if self.preferences["IMPORT_FURNITURES"]:
            self._import_elements(home, 'pieceOfFurniture')
            for furniture_group in home.findall('furnitureGroup'):
                self._import_elements(furniture_group, 'pieceOfFurniture', False)
            self._refresh()

        # Importing <light> elements ...
        if self.preferences["IMPORT_LIGHTS"]:
            self._import_elements(home, 'light')
            self._refresh()

        # Importing <observerCamera> elements ...
        if self.preferences["IMPORT_CAMERAS"]:
            self._import_elements(home, 'observerCamera')
            self._import_elements(home, 'camera')
            self._refresh()

        if self.preferences["CREATE_RENDER_PROJECT"] and self.site:
            Project.create(doc, renderer="Povray", template="povray_standard.pov")
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(self.site)
            Gui.runCommand('Render_View', 0)
            self._refresh()

        _msg(f"Successfully imported home '{home.get('name')}' ...")


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
            'DEBUG': get_param_arch("sh3dDebug"),
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
        }

    def _setup_handlers(self):
        self.handlers = {
            'level': LevelHandler(self),
            'room': RoomHandler(self),
            'wall': WallHandler(self),
        }
        if self.preferences["IMPORT_DOORS_AND_WINDOWS"]:
            self.handlers['doorOrWindow'] = DoorOrWindowHandler(self)

        if self.preferences["IMPORT_FURNITURES"]:
            self.handlers['pieceOfFurniture'] = FurnitureHandler(self)
            self.handlers['furnitureGroup'] = None

        if self.preferences["IMPORT_LIGHTS"]:
            self.handlers['light'] = LightHandler(self)

        if self.preferences["IMPORT_CAMERAS"]:
            camera_handler = CameraHandler(self)
            self.handlers['observerCamera'] = camera_handler
            self.handlers['camera'] = camera_handler

    def _refresh(self):
        App.ActiveDocument.recompute()
        if App.GuiUp:
            Gui.updateGui()

    def set_property(self, obj, type_, name, description, value, valid_values=None):
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

        self._add_property(obj, type_, name, description)
        if valid_values:
            setattr(obj, name, valid_values)
        if value is None:
            if self.preferences["DEBUG"]:_log(f"Setting obj.{name}=None")
            return
        if type(value) is ET.Element:
            if type_ == "App::PropertyString":
                value = str(value.get(name, ""))
            elif type_ == "App::PropertyFloat":
                value = float(value.get(name, 0))
            elif type_ == "App::PropertyInteger":
                value = int(value.get(name, 0))
            elif type_ == "App::PropertyBool":
                value = bool(value.get(name, True))
        if self.preferences["DEBUG"]:
            _log(f"Setting @{obj}.{name} = {value}")
        setattr(obj, name, value)

    def _add_property(self, obj, property_type, name, description):
        """Add an property to the FC object.

        All properties will be added under the 'SweetHome3D' group

        Args:
            obj (object): TheFC object to add a property to
            property_type (str): the type of property to add
            name (str): the name of the property to add
            description (str): a short description of the property to add
        """
        if name not in obj.PropertiesList:
            obj.addProperty(property_type, name, "SweetHome3D", description)

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
            if self.preferences["DEBUG"]:
                _log(translate("BIM", f"Merging imported element '{id}' with existing element of type '{type(fc_object)}'"))
            return fc_object
        if self.preferences["DEBUG"]:
            _log(translate("BIM", f"No element found with id '{id}' and type '{sh_type}'"))
        return None

    def add_floor(self, floor):
        self.floors[floor.id] = floor
        self.building.addObject(floor)

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

    def add_wall(self, wall):
        self.walls.append(wall)

    def _create_groups(self):
        """Create FreeCAD Group for the different imported elements
        """
        doc = App.ActiveDocument
        if self.preferences["IMPORT_LIGHTS"] and not doc.getObject("Lights"):
            _log(f"Creating Lights group ...")
            doc.addObject("App::DocumentObjectGroup", "Lights")
        if self.preferences["IMPORT_CAMERAS"] and not doc.getObject("Cameras"):
            _log(f"Creating Cameras group ...")
            doc.addObject("App::DocumentObjectGroup", "Cameras")

    def _setup_project(self, elm):
        """Create the Arch::Project and Arch::Site for this import

        Args:
            elm (str): the <home> element

        """
        if 'Project' in self.fc_objects:
            self.project = self.fc_objects.get('Project')
        elif self.preferences["CREATE_IFC_PROJECT"]:
            self.project = self._create_project()
        if 'Site' in self.fc_objects:
            self.site = self.fc_objects.get('Site')
        else:
            self.site = self._create_site()
        if elm.get('name') in self.fc_objects:
            self.building = self.fc_objects.get(elm.get('name'))
        else:
            self.building = self._create_building(elm)

        if self.preferences["CREATE_IFC_PROJECT"]:
            self.project.addObject(self.site)

        self.site.addObject(self.building)

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
        floor = Arch.makeFloor()
        floor.Label = 'Level'
        floor.Placement.Base.z = 0
        floor.Height = 2500

        self.set_property(floor, "App::PropertyString", "shType", "The element type", 'level')
        self.set_property(floor, "App::PropertyString", "id", "The element's id", 'Level')
        self.set_property(floor, "App::PropertyFloat", "floorThickness", "The floor's slab thickness", dim_fc2sh(floor.Height))
        if self.preferences["IMPORT_FURNITURES"]:
            group = floor.newObject("App::DocumentObjectGroup", "Furnitures")
            self.set_property(floor, "App::PropertyString", "FurnitureGroupName", "The DocumentObjectGroup name for all furnitures in this floor", group.Name)
            group = floor.newObject("App::DocumentObjectGroup", "Baseboards")
            self.set_property(floor, "App::PropertyString", "BaseboardGroupName", "The DocumentObjectGroup name for all baseboards on this floor", group.Name)

        return floor

    def _import_elements(self, parent, tag, update_progress=True):
        """Generic function to import a specific element.

        This function will lookup the handler registered for the elements
        `tag` and then call it on each item. It also provides some update
        on the whole process.

        Args:
            parent (Element): the parent of the elements to be imported.
                Usually the <home> element.
            tag (str): the tag of the elements to be imported.
            update_progress (bool, optional): whether to update the
                progress. Set to false when importing a group of elements.
                Defaults to True.
        """
        tags = list(self.handlers.keys())
        elements = parent.findall(tag)
        if update_progress and self.progress_bar:
            self.progress_bar.stop()
            self.progress_bar.start(f"Step {tags.index(tag)+1}/{len(tags)}: importing {len(elements)} '{tag}' elements. Please wait ...", len(elements))
            _msg(f"Importing {len(elements)} '{tag}' elements ...")
        def _process(tuple):
            (i, elm) = tuple
            _msg(f"Importing {tag}#{i} ({self.current_object_count + 1}/{self.total_object_count}) ...")
            try:
                self.handlers[tag].process(parent, i, elm)
            except Exception as e:
                _err(f"Failed to import <{tag}>#{i} ({elm.get('id', elm.get('name'))}):")
                _err(str(e))
            if update_progress and self.progress_bar:
                self.progress_bar.next()
            self.current_object_count = self.current_object_count + 1
        list(map(_process, enumerate(elements)))

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

        floor.ViewObject.Visibility = elm.get('visible', 'true') == 'true'

        if self.importer.preferences["IMPORT_FURNITURES"]:
            group = floor.newObject("App::DocumentObjectGroup", "Furnitures")
            self.setp(floor, "App::PropertyString", "FurnitureGroupName", "The DocumentObjectGroup name for all furnitures on this floor", group.Name)
            group = floor.newObject("App::DocumentObjectGroup", "Baseboards")
            self.setp(floor, "App::PropertyString", "BaseboardGroupName", "The DocumentObjectGroup name for all baseboards on this floor", group.Name)

        self.importer.add_floor(floor)

    def _set_properties(self, obj, elm):
        self.setp(obj, "App::PropertyString", "shType", "The element type", 'level')
        self.setp(obj, "App::PropertyString", "id", "The floor's id", elm)
        self.setp(obj, "App::PropertyFloat", "floorThickness", "The floor's slab thickness", dim_sh2fc(float(elm.get('floorThickness'))))
        self.setp(obj, "App::PropertyInteger", "elevationIndex", "The floor number", elm)
        self.setp(obj, "App::PropertyBool", "viewable", "Whether the floor is viewable", elm)


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
        level_id = elm.get('level', None)
        floor = self.get_floor(level_id)
        assert floor != None, f"Missing floor '{level_id}' for <room> '{elm.get('id')}' ..."

        points = []
        for point in elm.findall('point'):
            x = float(point.get('x'))
            y = float(point.get('y'))
            z = dim_fc2sh(floor.Placement.Base.z)
            points.append(coord_sh2fc(App.Vector(x, y, z)))

        slab = None
        if self.importer.preferences["MERGE"]:
            slab = self.get_fc_object(elm.get("id"), 'room')

        if not slab:
            line = Draft.make_wire(points, placement=App.Placement(), closed=True, face=True, support=None)
            slab = Arch.makeStructure(line, height=floor.floorThickness)

        slab.Label = elm.get('name', 'Room')
        slab.IfcType = "Slab"
        slab.Normal = -Z_NORM

        color = elm.get('floorColor', self.importer.preferences["DEFAULT_FLOOR_COLOR"])
        set_color_and_transparency(slab, color)
        self._set_properties(slab, elm)
        floor.addObject(slab)

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
        self.setp(obj, "App::PropertyFloat", "floorShininess", "The room's floor shininess", elm)
        self.setp(obj, "App::PropertyBool", "ceilingVisible", "Whether the ceiling of the room is displayed", elm)
        self.setp(obj, "App::PropertyString", "ceilingColor", "The room's ceiling color", ceiling_color)
        self.setp(obj, "App::PropertyFloat", "ceilingShininess", "The room's ceiling shininess", elm)
        self.setp(obj, "App::PropertyBool", "ceilingFlat", "", elm)


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
        assert floor != None, f"Missing floor '{level_id}' for <wall> '{elm.get('id')}' ..."

        wall = None
        if self.importer.preferences["MERGE"]:
            wall = self.get_fc_object(elm.get("id"), 'wall')

        if not wall:
            prev = self._get_sibling_wall(parent, elm, 'wallAtStart')
            next = self._get_sibling_wall(parent, elm, 'wallAtEnd')
            wall = self._create_wall(floor, prev, next, elm)
            if not wall:
                _log(f"No wall created for {elm.get('id')}. Skipping!")
                return

        self._set_wall_colors(wall, elm)

        wall.IfcType = "Wall"
        wall.Label = f"wall{i}"

        self._set_properties(wall, elm)

        floor.addObject(wall)
        self.importer.add_wall(wall)

        if self.importer.preferences["IMPORT_FURNITURES"]:
            App.ActiveDocument.recompute([wall])
            for baseboard in elm.findall('baseboard'):
                self._import_baseboard(floor, wall, baseboard)

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
        self.setp(obj, "App::PropertyString", "shType", "The element type", 'wall')
        self.setp(obj, "App::PropertyString", "id", "The wall's id", elm)
        self.setp(obj, "App::PropertyString", "wallAtStart", "The Id of the contiguous wall at the start of this wall", elm)
        self.setp(obj, "App::PropertyString", "wallAtEnd", "The Id of the contiguous wall at the end of this wall", elm)
        self.setp(obj, "App::PropertyString", "pattern", "The pattern of this wall in plan view", elm)
        self.setp(obj, "App::PropertyFloat", "leftSideShininess", "The wall's left hand side shininess", elm)
        self.setp(obj, "App::PropertyFloat", "rightSideShininess", "The wall's right hand side shininess", elm)

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
        wall_details = self._get_wall_details(floor, elm)
        assert wall_details is not None, f"Fail to get details of wall {elm.get('id')}. Bailing out! {elm} / {wall_details}"

        # Both the wall at start or the wall at end can be None.
        prev_wall_details = self._get_wall_details(floor, prev)
        next_wall_details = self._get_wall_details(floor, next)

        # Is the wall curved (i.e. arc_extent != 0) ?
        if wall_details[5] != 0:
            section_start, section_end, spine = self._create_curved_segment(
                wall_details,
                prev_wall_details,
                next_wall_details)
        else:
            section_start, section_end, spine = self._create_straight_segment(
                wall_details,
                prev_wall_details,
                next_wall_details)

        sweep = App.ActiveDocument.addObject('Part::Sweep')
        sweep.Sections = [section_start, section_end]
        sweep.Spine = spine
        sweep.Solid = True
        sweep.Frenet = False
        section_start.Visibility = False
        section_end.Visibility = False
        spine.Visibility = False
        App.ActiveDocument.recompute([sweep])
        # Sometimes the Part::Sweep creates a "twisted" sweep which
        # result in a broken wall. The solution is to use a compound
        # object based on ruled surface instead.
        if FIX_INVALID_SWEEP and (sweep.Shape.isNull() or not sweep.Shape.isValid()):
            _log(f"Part::Sweep for wall#{elm.get('id')} is invalid. Using ruled surface instead ...")
            ruled_surface = App.ActiveDocument.addObject('Part::RuledSurface')
            ruled_surface.Curve1 = section_start
            ruled_surface.Curve2 = section_end
            App.ActiveDocument.recompute([ruled_surface])
            _log(f"Creating compound object ...")
            compound = App.ActiveDocument.addObject('Part::Compound')
            compound.Links = [ruled_surface, section_start, section_end]
            App.ActiveDocument.recompute([compound])
            _log(f"Creating solid ...")
            solid = App.ActiveDocument.addObject("Part::Feature")
            solid.Shape = Part.Solid(Part.Shell(compound.Shape.Faces))
            doc = App.ActiveDocument
            doc.removeObject(compound.Label)
            doc.recompute()
            doc.removeObject(ruled_surface.Label)
            doc.recompute()
            doc.removeObject(sweep.Label)
            doc.recompute()
            doc.removeObject(spine.Label)
            doc.recompute()
            doc.removeObject(section_start.Label)
            doc.removeObject(section_end.Label)
            wall = Arch.makeWall(solid)
        else:
            wall = Arch.makeWall(sweep)
        # For some reason the Length of the spine is not propagated to the
        # wall itself...
        wall.Length = spine.Length
        return wall

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

        section_start = self._get_section(wall_details, True, prev_wall_details)
        section_end = self._get_section(wall_details, False, next_wall_details)

        spine = Draft.makeLine(start, end)
        App.ActiveDocument.recompute([section_start, section_end, spine])
        if self.importer.preferences["DEBUG"]:
            _log(f"_create_straight_segment(): wall {self._pv(start)}->{self._pv(end)} => section_start={self._ps(section_start)}, section_end={self._ps(section_end)}")

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
        (start, end, _, _, _, arc_extent) = wall_details

        section_start = self._get_section(wall_details, True, prev_wall_details)
        section_end = self._get_section(wall_details, False, next_wall_details)

        a1, a2, (invert_angle, center, radius) = self._get_normal_angles(wall_details)

        placement = App.Placement(center, App.Rotation())
        # BEWARE: makeCircle always draws counter-clockwise (i.e. in positive
        # direction in xYz coordinate system). We therefore need to invert
        # the start and end angle (as in SweetHome the wall is drawn in
        # clockwise fashion).
        if invert_angle:
            spine = Draft.makeCircle(radius, placement, False, a1, a2)
        else:
            spine = Draft.makeCircle(radius, placement, False, a2, a1)

        App.ActiveDocument.recompute([section_start, section_end, spine])
        if self.importer.preferences["DEBUG"]:
            _log(f"_create_curved_segment(): wall {self._pv(start)}->{self._pv(end)} => section_start={self._ps(section_start)}, section_end={self._ps(section_end)}")

        return section_start, section_end, spine

    def _get_section(self, wall_details, at_start, sibling_details):
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

            if self.importer.preferences["DEBUG"]:
                _log(f"Joining wall {self._pv(end-start)}@{self._pv(start)} and wall {self._pv(s_end-s_start)}@{self._pv(s_start)}")
                _log(f"    wall: {self._pe(lside)},{self._pe(rside)}")
                _log(f" sibling: {self._pe(s_lside)},{self._pe(s_rside)}")
                _log(f"intersec: {self._pv(i_start)},{self._pv(i_end)}")
            section = Draft.makeRectangle([i_start, i_end, i_end_z, i_start_z])
            if self.importer.preferences["DEBUG"]:
                _log(f"section: {section}")
        else:
            (start, end, thickness, height_start, height_end, _) = wall_details
            height = height_start if at_start else height_end
            center = start if at_start else end
            a1, a2, _ = self._get_normal_angles(wall_details)
            z_rotation = a1 if at_start else a2
            section = Draft.makeRectangle(thickness, height)
            Draft.move([section], App.Vector(-thickness/2, 0, 0))
            Draft.rotate([section], 90, ORIGIN, X_NORM)
            Draft.rotate([section], z_rotation, ORIGIN, Z_NORM)
            Draft.move([section], center)

        if self.importer.preferences["DEBUG"]:
            App.ActiveDocument.recompute()
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
        (start, end, thickness, height_start, height_end, arc_extent) = wall_details

        angle_start = angle_end = 0
        invert_angle = False
        center = radius = None
        if arc_extent == 0:
            angle_start = angle_end = 90-math.degrees(DraftVecUtils.angle(end-start, X_NORM))
        else:
            # Calculate the circle that pases through the center of both rectangle
            #   and has the correct angle between p1 and p2
            chord = DraftVecUtils.dist(start, end)
            radius = abs(chord / (2*math.sin(arc_extent/2)))

            circles = DraftGeomUtils.circleFrom2PointsRadius(start, end, radius)
            # We take the center that preserve the arc_extent orientation (in FC
            #   coordinate). The orientation is calculated from start to end
            center = circles[0].Center
            if numpy.sign(arc_extent) != numpy.sign(DraftVecUtils.angle(start-center, end-center, Z_NORM)):
                invert_angle = True
                center = circles[1].Center

            # radius1 and radius2 are the vector from center to start and end respectively
            radius1 = start - center
            radius2 = end - center

            angle_start = math.degrees(DraftVecUtils.angle(X_NORM, radius1, Z_NORM))
            angle_end = math.degrees(DraftVecUtils.angle(X_NORM, radius2, Z_NORM))

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
        if self.importer.preferences["DEBUG"]:
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

    def _ps(self, section):
        # Pretty print a Section in a condensed way
        v = section.Shape.Vertexes
        return f"[{self._pv(v[0].Point)}, {self._pv(v[1].Point)}, {self._pv(v[2].Point)}, {self._pv(v[3].Point)}]"

    def _pe(self, edge):
        # Print an Edge in a condensed way
        v = edge.Vertexes
        return f"[{self._pv(v[0].Point)}, {self._pv(v[1].Point)}]"

    def _pv(self, vect):
        # Print an Vector in a condensed way
        return f"({round(getattr(vect, 'X', getattr(vect,'x')))},{round(getattr(vect, 'Y', getattr(vect,'y')))})"

    def _set_wall_colors(self, wall, elm):
        """Set the `wall`'s color taken from `elm`.

        Using `ViewObject.DiffuseColor` attribute to set the different
        color faces. Note that when the faces are changing (i.e. when
        adding doors & windows). This will generate the wrong color
        """
        topColor = elm.get('topColor', self.importer.preferences["DEFAULT_FLOOR_COLOR"])
        set_color_and_transparency(wall, topColor)
        leftSideColor = hex2rgb(elm.get('leftSideColor', topColor))
        rightSideColor = hex2rgb(elm.get('rightSideColor', topColor))
        topColor = hex2rgb(topColor)
        diffuse_color = [topColor, leftSideColor, topColor, rightSideColor, topColor, topColor]
        if ang_sh2fc(elm.get('arcExtent', 0)) > 0:
            diffuse_color = [topColor, rightSideColor, topColor, leftSideColor, topColor, topColor]

        if hasattr(wall.ViewObject, "DiffuseColor"):
            wall.ViewObject.DiffuseColor = diffuse_color

    def _import_baseboard(self, floor, wall, elm):
        """Creates and returns a Part::Extrusion from the imported_baseboard object

        Args:
            floor (Slab): the Slab the wall belongs to
            wall (Wall): the Arch wall
            elm (Element): the wall being imported

        Returns:
            Part::Extrusion: the newly created object
        """
        wall_width = float(wall.Width)
        baseboard_width = dim_sh2fc(elm.get('thickness'))
        baseboard_height = dim_sh2fc(elm.get('height'))
        vertexes = wall.Shape.Vertexes

        # The left side is defined as the face on the left hand side when going
        # from (xStart,yStart) to (xEnd,yEnd). Assume the points are always
        # created in the same order. We then have on the lefthand side the points
        # 1 and 2, while on the righthand side we have the points 4 and 6
        side = elm.get('attribute')
        if side == 'leftSideBaseboard':
            p_start = vertexes[0].Point
            p_end = vertexes[2].Point
            p_normal = vertexes[4].Point
        elif side == 'rightSideBaseboard':
            p_start = vertexes[4].Point
            p_end = vertexes[6].Point
            p_normal = vertexes[0].Point
        else:
            raise ValueError(f"Invalid SweetHome3D file: invalid baseboard with 'attribute'={side}")

        v_normal = p_normal - p_start
        v_baseboard = v_normal * (baseboard_width/wall_width)
        p0 = p_start
        p1 = p_end
        p2 = p_end - v_baseboard
        p3 = p_start - v_baseboard

        baseboard_id = f"{wall.id}-{side}"
        baseboard = None
        if self.importer.preferences["MERGE"]:
            baseboard = self.get_fc_object(baseboard_id, 'baseboard')

        if not baseboard:
            # I first add a rectangle
            base = Draft.makeRectangle([p0, p1, p2, p3], face=True, support=None)
            base.Visibility = False
            # and then I extrude
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

        set_color_and_transparency(baseboard, elm.get('color'))

        self.setp(baseboard, "App::PropertyString", "shType", "The element type", 'baseboard')
        self.setp(baseboard, "App::PropertyString", "id", "The element's id", baseboard_id)
        self.setp(baseboard, "App::PropertyLink", "parent", "The element parent", wall)

        if 'BaseboardGroupName' not in floor.PropertiesList:
            group = floor.newObject("App::DocumentObjectGroup", "Baseboards")
            self.setp(floor, "App::PropertyString", "BaseboardGroupName", "The DocumentObjectGroup name for all baseboards on this floor", group.Name)

        floor.getObject(floor.BaseboardGroupName).addObject(baseboard)


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
        self.setp(obj, "App::PropertyFloat", "shininess", "The object's shininess", elm)
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
        assert floor != None, f"Missing floor '{level_id}' for <doorOrWindow> '{door_id}' ..."


        feature = None
        if self.importer.preferences["MERGE"]:
            feature = self.get_fc_object(door_id, 'doorOrWindow')

        if not feature:
            feature = self._create_door(floor, elm)

        assert feature != None, f"Missing feature for <doorOrWindow> {door_id} ..."

        self._set_properties(feature, elm)
        self.set_furniture_common_properties(feature, elm)
        self.set_piece_of_furniture_common_properties(feature, elm)
        self.setp(feature, "App::PropertyString", "id", "The furniture's id", door_id)

    def _set_properties(self, obj, elm):
        self.setp(obj, "App::PropertyString", "shType", "The element type", 'doorOrWindow')
        self.setp(obj, "App::PropertyFloat", "wallThickness", "", float(elm.get('wallThickness', 1)))
        self.setp(obj, "App::PropertyFloat", "wallDistance", "", elm)
        self.setp(obj, "App::PropertyFloat", "wallWidth", "", float(elm.get('wallWidth', 1)))
        self.setp(obj, "App::PropertyFloat", "wallLeft", "", elm)
        self.setp(obj, "App::PropertyFloat", "wallHeight", "", float(elm.get('wallHeight', 1)))
        self.setp(obj, "App::PropertyFloat", "wallTop", "", elm)
        self.setp(obj, "App::PropertyBool", "wallCutOutOnBothSides", "", elm)
        self.setp(obj, "App::PropertyBool", "widthDepthDeformable", "", elm)
        self.setp(obj, "App::PropertyString", "cutOutShape", "", elm)
        self.setp(obj, "App::PropertyBool", "boundToWall", "", elm)

    def _create_door(self, floor, elm):
        # The window in SweetHome3D is defined with a width, depth, height.
        # Furthermore the (x.y.z) is the center point of the lower face of the
        # window. In FC the placement is defined on the face of the whole that
        # will contain the windows. The makes this calculation rather
        # cumbersome.
        x_center = float(elm.get('x'))
        y_center = float(elm.get('y'))
        z_center = float(elm.get('elevation', 0))
        z_center += dim_fc2sh(floor.Placement.Base.z)

        # This is the FC coordinate of the center point of the lower face of the
        # window. This then needs to be moved to the proper face on the wall and
        # offset properly with respect to the wall's face.
        center = coord_sh2fc(App.Vector(x_center, y_center, z_center))

        wall_width = -DEFAULT_WALL_WIDTH
        wall = self._get_wall(center)
        if wall:
            wall_width = wall.Width
        else:
            _err(f"Missing wall for <doorOrWindow> {elm.get('id')}. Defaulting to width {DEFAULT_WALL_WIDTH} ...")

        width = dim_sh2fc(elm.get('width'))
        depth = dim_sh2fc(elm.get('depth'))
        height = dim_sh2fc(elm.get('height'))
        angle = float(elm.get('angle', 0))
        mirrored = bool(elm.get('modelMirrored', False))

        # this is the vector that allow me to go from the center to the corner
        # of the bounding box. Note that the angle of the rotation is negated
        # because the y axis is reversed in SweetHome3D
        center2corner = App.Vector(-width/2, -wall_width/2, 0)
        rotation = App.Rotation(App.Vector(0, 0, 1), math.degrees(-angle))
        center2corner = rotation.multVec(center2corner)

        corner = center.add(center2corner)
        pl = App.Placement(
            corner,  # translation
            App.Rotation(math.degrees(-angle), 0, 90),  # rotation
            ORIGIN  # rotation@coordinate
        )

        # NOTE: the windows are not imported as meshes, but we use a simple
        #   correspondence between a catalog ID and a specific window preset from
        #   the parts library.
        catalog_id = elm.get('catalogId')
        (windowtype, ifc_type) = DOOR_MODELS.get(catalog_id, (None, None))
        if not windowtype:
            _wrn(f"Unknown catalogId {catalog_id} for element {elm.get('id')}. Defaulting to 'Simple Door'")
            (windowtype, ifc_type) = ('Simple door', 'Door')

        h1 = 10
        h2 = 10
        h3 = 0
        w1 = min(depth, wall_width)
        w2 = 10
        o1 = 0
        o2 = w1 / 2
        window = Arch.makeWindowPreset(windowtype, width, height, h1, h2, h3, w1, w2, o1, o2, pl)
        window.IfcType = ifc_type
        if ifc_type == 'Door' and mirrored:
            window.OperationType = "SINGLE_SWING_RIGHT"

        # Adjust symbol plan, Sweet Home has the opening in the opposite side by default
        window.ViewObject.Proxy.invertOpening()
        if mirrored:
            window.ViewObject.Proxy.invertHinge()

        if wall:
            window.Hosts = [wall]
        return window

    def _get_wall(self, point):
        """Returns the wall that contains the given point.

        Args:
            point (FreeCAD.Vector): the point to test for

        Returns:
            Arch::Wall: the wall that contains the given point
        """
        for wall in self.importer.walls:
            try:
                if wall.Shape.BoundBox.isInside(point):
                    return wall
            except FloatingPointError:
                pass
        return None


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
        furniture_id = f"{elm.get('id', elm.get('name'))}-{i}"
        level_id = elm.get('level', None)
        floor = self.get_floor(level_id)
        assert floor != None, f"Missing floor '{level_id}' for <pieceOfFurniture> '{furniture_id}' ..."

        feature = None
        if self.importer.preferences["MERGE"]:
            feature = self.get_fc_object(furniture_id, 'pieceOfFurniture')

        if not feature:
            feature = self._create_equipment(elm)
        self.setp(feature, "App::PropertyString", "shType", "The element type", 'pieceOfFurniture')
        self.set_furniture_common_properties(feature, elm)
        self.set_piece_of_furniture_common_properties(feature, elm)
        self.set_piece_of_furniture_horizontal_rotation_properties(feature, elm)
        self.setp(feature, "App::PropertyString", "id", "The furniture's id", furniture_id)

        if 'FurnitureGroupName' not in floor.PropertiesList:
            group = floor.newObject("App::DocumentObjectGroup", "Furnitures")
            self.setp(floor, "App::PropertyString", "FurnitureGroupName", "The DocumentObjectGroup name for all furnitures on this floor", group.Name)

        floor.getObject(floor.FurnitureGroupName).addObject(feature)

        # We add the object to the list of known object that can then
        # be referenced elsewhere in the SH3D model (i.e. lights).
        self.importer.fc_objects[feature.id] = feature

    def _create_equipment(self, elm):

        floor = self.get_floor(elm.get('level'))

        width = dim_sh2fc(float(elm.get('width')))
        depth = dim_sh2fc(float(elm.get('depth')))
        height = dim_sh2fc(float(elm.get('height')))
        x = float(elm.get('x', 0))
        y = float(elm.get('y', 0))
        z = float(elm.get('elevation', 0.0))
        angle = float(elm.get('angle', 0.0))
        pitch = float(elm.get('pitch', 0.0))  # X Axis
        roll = float(elm.get('roll', 0.0))  # Y Axis
        name = elm.get('name')
        mirrored = bool(elm.get('modelMirrored', "false") == "true")

        # The meshes are normalized, facing up.
        # Center, Scale, X Rotation && Z Rotation (in FC axes), Move
        mesh = self._get_mesh(elm)
        bb = mesh.BoundBox
        transform = App.Matrix()
        transform.move(-bb.Center)
        # NOTE: the model is facing up, thus y and z are inverted
        transform.scale(width/bb.XLength, height/bb.YLength, depth/bb.ZLength)
        transform.rotateX(math.pi/2)
        transform.rotateX(-pitch)
        transform.rotateY(roll)
        transform.rotateZ(-angle)
        level_elevation = dim_fc2sh(floor.Placement.Base.z)
        distance = App.Vector(x, y, level_elevation + z + (dim_fc2sh(height) / 2))
        transform.move(coord_sh2fc(distance))
        mesh.transform(transform)

        if self.importer.preferences["CREATE_ARCH_EQUIPMENT"]:
            shape = Part.Shape()
            shape.makeShapeFromMesh(mesh.Topology, 0.100000)
            equipment = Arch.makeEquipment(name=name)
            equipment.Shape = shape
            equipment.purgeTouched()
        else:
            equipment = App.ActiveDocument.addObject("Mesh::Feature", name)
            equipment.Mesh = mesh

        return equipment


class LightHandler(FurnitureHandler):
    """A helper class to import a SH3D `<light>` object."""

    def __init__(self, importer: SH3DImporter):
        super().__init__(importer)

    def process(self, parent, i, elm):
        """_summary_

        Args:
            i (int): the ordinal of the imported element
            elm (Element): the xml element
        """
        light_id = f"{elm.get('id', elm.get('name'))}-{i}"
        level_id = elm.get('level', None)
        floor = self.get_floor(level_id)
        assert floor != None, f"Missing floor '{level_id}' for <doorOrWindow> '{light_id}' ..."

        if self.importer.preferences["IMPORT_FURNITURES"]:
            super().process(i, elm)
            light_apppliance = self.get_fc_object(light_id, 'pieceOfFurniture')
            assert light_apppliance != None, f"Missing <light> furniture {light_id} ..."
            self.setp(light_apppliance, "App::PropertyFloat", "power", "The power of the light",  float(elm.get('power', 0.5)))

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
            self.setp(light_source, "App::PropertyLink", "lightAppliance", "The furniture", light_apppliance)

            App.ActiveDocument.Lights.addObject(light_source)


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

        camera_id = f"{attribute}-{i}"
        camera = None
        if self.importer.preferences["MERGE"]:
            camera = self.get_fc_object(camera_id, attribute)

        if not camera:
            _, camera, _ = Camera.create()
            App.ActiveDocument.Cameras.addObject(camera)

        # ¿How to convert fov to FocalLength?
        fieldOfView = float(elm.get('fieldOfView'))
        fieldOfView = math.degrees(fieldOfView)

        camera.Label = elm.get('name', attribute.title())
        camera.Placement.Base = coord_sh2fc(App.Vector(x, y, z))
        # NOTE: the coordinate system is screen like, thus roll & picth are inverted ZY'X''
        camera.Placement.Rotation.setYawPitchRoll(
            math.degrees(math.pi-yaw), 0, math.degrees(math.pi/2-pitch))
        camera.Projection = "Perspective"
        camera.AspectRatio = 1.33333333  # /home/environment/@photoAspectRatio

        self._set_properties(camera, elm)

    def _set_properties(self, obj, elm):
        self.setp(obj, "App::PropertyString", "shType", "The element type", 'camera')
        self.setp(obj, "App::PropertyString", "id", "The object ID", elm)
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


def ang_sh2fc(angle):
    """Convert SweetHome angle (º) to FreeCAD angle (º)

    SweetHome angles are clockwise positive while FreeCAD are anti-clockwise
    positive

    Args:
        angle (float): The angle in SweetHome

    Returns:
        float: the FreeCAD angle
    """
    return -float(angle)


def set_color_and_transparency(obj, color):
    if not App.GuiUp or not color:
        return
    if hasattr(obj.ViewObject, "ShapeColor"):
        obj.ViewObject.ShapeColor = hex2rgb(color)
    if hasattr(obj.ViewObject, "Transparency"):
        obj.ViewObject.Transparency = _hex2transparency(color)


def color_fc2sh(hexcode):
    # 0xRRGGBBAA => AARRGGBB
    hex_str = hex(int(hexcode))[2:]
    return ''.join([hex_str[6:], hex_str[0:6]])


def hex2rgb(hexcode):
    # We might have transparency as the first 2 digit
    offset = 0 if len(hexcode) == 6 else 2
    return (
        int(hexcode[offset:offset+2], 16),   # Red
        int(hexcode[offset+2:offset+4], 16),  # Green
        int(hexcode[offset+4:offset+6], 16)  # Blue
        )


def _hex2transparency(hexcode):
    return 100 - int(int(hexcode[0:2], 16) * 100 / 255)
