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
import itertools
import numpy as np
import math
import os
import re
import traceback
import uuid
import xml.etree.ElementTree as ET
import zipfile

import Arch
import BOPTools.SplitFeatures
import BOPTools.BOPFeatures
import Draft
import DraftGeomUtils
import DraftVecUtils
import Mesh
import MeshPart
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

# SweetHome3D is in cm while FreeCAD is in mm
FACTOR = 10
DEFAULT_WALL_WIDTH = 100
TOLERANCE = float(.1)
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

# The Windows lookup map. This is really brittle and a better system should
# be found. Arch.WindowPresets =  ["Fixed", "Open 1-pane", "Open 2-pane",
#       "Sash 2-pane", "Sliding 2-pane", "Simple door", "Glass door",
#       "Sliding 4-pane", "Awning"]
# unzip -p all-windows.sh3d Home.xml | \
#   grep 'catalogId=' | \
#   sed -e 's/.*catalogId=//;s/ name=.*/: ("Fixed","Window"),/' | sort -u
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

    'eTeks#doubleFrenchWindow126x200': ("Fixed","Window"),
    'eTeks#doubleHungWindow80x122': ("Fixed","Window"),
    'eTeks#doubleOutwardOpeningWindow': ("Fixed","Window"),
    'eTeks#doubleWindow126x123': ("Fixed","Window"),
    'eTeks#doubleWindow126x163': ("Fixed","Window"),
    'eTeks#fixedTriangleWindow85x85': ("Fixed","Window"),
    'eTeks#fixedWindow85x123': ("Fixed","Window"),
    'eTeks#frenchWindow85x200': ("Fixed","Window"),
    'eTeks#halfRoundWindow': ("Fixed","Window"),
    'eTeks#roundWindow': ("Fixed","Window"),
    'eTeks#sliderWindow126x200': ("Fixed","Window"),
    'eTeks#window85x123': ("Fixed","Window"),
    'eTeks#window85x163': ("Fixed","Window"),
    'Kator Legaz#window-01': ("Fixed","Window"),
    'Kator Legaz#window-08-02': ("Fixed","Window"),
    'Kator Legaz#window-08': ("Fixed","Window"),
    'Scopia#turn-window': ("Fixed","Window"),
    'Scopia#window_2x1_medium_with_large_pane': ("Fixed","Window"),
    'Scopia#window_2x1_with_sliders': ("Fixed","Window"),
    'Scopia#window_2x3_arched': ("Fixed","Window"),
    'Scopia#window_2x3': ("Fixed","Window"),
    'Scopia#window_2x3_regular': ("Fixed","Window"),
    'Scopia#window_2x4_arched': ("Fixed","Window"),
    'Scopia#window_2x4': ("Fixed","Window"),
    'Scopia#window_2x6': ("Fixed","Window"),
    'Scopia#window_3x1': ("Fixed","Window"),
    'Scopia#window_4x1': ("Fixed","Window"),
    'Scopia#window_4x3_arched': ("Fixed","Window"),
    'Scopia#window_4x3': ("Fixed","Window"),
    'Scopia#window_4x5': ("Fixed","Window"),

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
            if self.preferences["DEBUG"]: _log("No level defined. Using default level ...")
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

        # Door&Windows have been imported. Now we can decorate...
        if self.preferences["DECORATE_SURFACES"]:
            self._decorate_surfaces()
            self._refresh()

        # Importing <pieceOfFurniture> && <furnitureGroup> elements ...
        if self.preferences["IMPORT_FURNITURES"]:
            self._import_elements(home, ET_XPATH_PIECE_OF_FURNITURE)
            self._refresh()

        # Importing <light> elements ...
        if self.preferences["IMPORT_LIGHTS"]:
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
            'CREATE_GROUND_MESH': get_param_arch("sh3dCreateGroundMesh"),
            'DEFAULT_GROUND_COLOR': color_fc2sh(get_param_arch("sh3dDefaultGroundColor")),
            'DEFAULT_SKY_COLOR': color_fc2sh(get_param_arch("sh3dDefaultSkyColor")),
            'DECORATE_SURFACES': get_param_arch("sh3dDecorateSurfaces"),
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

        if self.preferences["IMPORT_LIGHTS"]:
            self.handlers[ET_XPATH_LIGHT] = LightHandler(self)

        if self.preferences["IMPORT_CAMERAS"]:
            camera_handler = CameraHandler(self)
            self.handlers[ET_XPATH_OBSERVER_CAMERA] = camera_handler
            self.handlers[ET_XPATH_CAMERA] = camera_handler

    def _refresh(self):
        App.ActiveDocument.recompute()
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
            if self.preferences["DEBUG"]:_log(f"Setting obj.{name}=None")
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
        if self.preferences["DEBUG"]:
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
            obj.addProperty(property_type, name, group, description)

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

    def add_space(self, floor, space):
        if floor.id not in self.spaces:
            self.spaces[floor.id] = []
        self.spaces[floor.id].append(space)
    
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
            space_z = space_face.CenterOfMass.z
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
        # ground = App.ActiveDocument.addObject("Part::Feature", "Ground")
        ground_face = Part.makeFace([ Part.Wire([edge0, edge1, edge2, edge3]) ])

        ground =  App.ActiveDocument.addObject("Mesh::Feature", "Ground")
        ground.Mesh = MeshPart.meshFromShape(Shape=ground_face, LinearDeflection=0.1, AngularDeflection=0.523599, Relative=False)
        ground.Label = "Ground"

        set_color_and_transparency(ground, self.site.groundColor)
        ground.ViewObject.Transparency = 50
        # TODO: apply possible <texture> within the <environment> element

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
        xpaths = list(self.handlers.keys())
        elements = parent.findall(xpath)
        tag_name = xpath[3:] if xpath.startswith('.') else xpath

        total_steps, current_step, total_elements = self._get_progress_info(xpath, elements)
        if self.progress_bar:
            self.progress_bar.stop()
            self.progress_bar.start(f"Step {current_step}/{total_steps}: importing {total_elements} '{tag_name}' elements. Please wait ...", total_elements)
            _msg(f"Importing {total_elements} '{tag_name}' elements ...")
        def _process(tuple):
            (i, elm) = tuple
            _msg(f"Importing {tag_name}#{i} ({self.current_object_count + 1}/{self.total_object_count}) ...")
            try:
                self.handlers[xpath].process(parent, i, elm)
            except Exception as e:
                _err(f"Failed to import <{tag_name}>#{i} ({elm.get('id', elm.get('name'))}):")
                _err(str(e))
                _err(traceback.format_exc())
            if self.progress_bar:
                self.progress_bar.next()
            self.current_object_count = self.current_object_count + 1
        list(map(_process, enumerate(elements)))

    def _get_progress_info(self, xpath, elements):
        xpaths = list(self.handlers.keys())
        total_steps = len(xpaths)
        current_step = xpaths.index(xpath)+1
        return total_steps, current_step, len(elements)

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
            self.site.Declination = ang_sh2fc(math.degrees(float(self.site.northDirection)))
            self.site.Longitude = math.degrees(float(self.site.longitude))
            self.site.Latitude = math.degrees(float(self.site.latitude))
            self.site.EPWFile = '' # https://www.ladybug.tools/epwmap/ or https://climate.onebuilding.org
        else:
            _msg(f"No <compass> tag found in <{elm.tag}>")

    def _create_slabs(self):
        floors = self.floors.values()
        total_steps, current_step, total_elements = self._get_progress_info(ET_XPATH_DUMMY_SLAB, floors)
        if self.progress_bar:
            self.progress_bar.stop()
            self.progress_bar.start(f"Step {current_step}/{total_steps}: Creating {total_elements} 'slab' elements. Please wait ...", total_elements)
            _msg(f"Creating {total_elements} 'slab' elements ...")
        handler = self.handlers[ET_XPATH_LEVEL]
        def _create_slab(tuple):
            (i, floor) = tuple
            _msg(f"Creating slab#{i} for floor '{floor.Label}' ...")
            try:
                handler.create_slabs(floor)
            except Exception as e:
                _err(f"Failed to create slab#{i} for floor '{floor.Label}':")
                _err(str(e))
                _err(traceback.format_exc())
            if self.progress_bar:
                self.progress_bar.next()
        list(map(_create_slab, enumerate(floors)))

    def _decorate_surfaces(self):

        all_spaces = self.spaces.values()
        all_spaces = list(itertools.chain(*all_spaces))
        all_walls = self.walls.values()
        all_walls = list(itertools.chain(*all_walls))

        total_elements = len(all_spaces)+len(all_walls)

        if self.progress_bar:
            self.progress_bar.stop()
            self.progress_bar.start(f"Decorating {total_elements} elements. Please wait ...", total_elements)
        _msg(f"Decorating {total_elements} elements ...")

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

    def _pv(self, v, print_z: bool = False, ndigits: None = None):
        # Print an Vector in a condensed way
        if hasattr(v,'X'):
            return f"({round(getattr(v, 'X'), ndigits)},{round(getattr(v, 'Y'), ndigits)}{',' + str(round(getattr(v, 'Z'), ndigits)) if print_z else ''})"
        elif hasattr(v,'x'):
            return f"({round(getattr(v, 'x'), ndigits)},{round(getattr(v, 'y'), ndigits)}{',' + str(round(getattr(v, 'z'), ndigits)) if print_z else ''})"
        raise ValueError(f"Expected a Point or Vector, got {type(v)}")


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
        group = floor.newObject("App::DocumentObjectGroup")
        group.Label = f"References-{floor.Label}"
        self.setp(floor, "App::PropertyString", "ReferenceFacesGroupName", "The DocumentObjectGroup name for all Reference Faces on this floor", group.Name)
        group.Visibility = False
        group.ViewObject.ShowInTree = False

        if self.importer.preferences["DECORATE_SURFACES"]:
            group = floor.newObject("App::DocumentObjectGroup")
            group.Label = f"Decoration-{floor.Label}-Walls"
            self.setp(floor, "App::PropertyString", "DecorationWallsGroupName", "The DocumentObjectGroup name for all wall decorations on this floor", group.Name)
            group = floor.newObject("App::DocumentObjectGroup")
            group.Label = f"Decoration-{floor.Label}-Ceilings"
            self.setp(floor, "App::PropertyString", "DecorationCeilingsGroupName", "The DocumentObjectGroup name for all ceilings decoration on this floor", group.Name)
            group = floor.newObject("App::DocumentObjectGroup")
            group.Label = f"Decoration-{floor.Label}-Floors"
            self.setp(floor, "App::PropertyString", "DecorationFloorsGroupName", "The DocumentObjectGroup name for all floors decoration on this floor", group.Name)
            group = floor.newObject("App::DocumentObjectGroup")
            group.Label = f"Decoration-{floor.Label}-Baseboards"
            self.setp(floor, "App::PropertyString", "DecorationBaseboardsGroupName", "The DocumentObjectGroup name for all baseboards on this floor", group.Name)

        if self.importer.preferences["IMPORT_FURNITURES"]:
            group = floor.newObject("App::DocumentObjectGroup", f"Furnitures-{floor.Label}")
            self.setp(floor, "App::PropertyString", "FurnitureGroupName", "The DocumentObjectGroup name for all furnitures in this floor", group.Name)

    def create_slabs(self, floor):
        """Creates a Arch.Slab for the given floor.

        Creating a slab consists in projecting all the structures of that
        floor into a plane, then create a extrusion for each one and then
        fuse thogether (in order to simplify the slab geometry).

        Args:
            floor (Arch.Floor): the Arch Floor for which to create the Slab
        """
        # Take the walls and only the spaces whose floor is actually visible.
        objects_to_project = list(filter(lambda s: s.floorVisible, self.get_spaces(floor)))
        objects_to_project.extend(self.get_walls(floor))
        objects_to_fuse = self._get_object_to_fuse(floor, objects_to_project)
        if len(objects_to_fuse) > 0:
            if len(objects_to_fuse) > 1:
                bf = BOPTools.BOPFeatures.BOPFeatures(App.ActiveDocument)
                slab_base = bf.make_multi_fuse([ o.Name for o in objects_to_fuse])
                slab_base.Label = f"{floor.Label}-footprint"
            else:
                slab_base = objects_to_fuse[0]
                slab_base.Label = f"{floor.Label}-footprint"

            slab = Arch.makeStructure(slab_base)
            slab.Label = f"{floor.Label}-slab"
            slab.setExpression('Height', f"{slab_base.Name}.Shape.BoundBox.ZLength")
            slab.Normal = -Z_NORM
            floor.addObject(slab)
        else:
            _wrn(f"No object found for floor {floor.Label}. No slab created.")

    def _get_object_to_fuse(self, floor, objects_to_project):
        group = floor.newObject("App::DocumentObjectGroup", f"SlabObjects-{floor.Label}")
        group.Visibility = False
        group.ViewObject.ShowInTree = False

        objects_to_fuse = []
        for object in objects_to_project:
            # Project the floor's objects onto the XY plane
            sv = Draft.make_shape2dview(object, Z_NORM)
            sv.Label = f"SV-{floor.Label}-{object.Label}"
            sv.Placement.Base.z = floor.Placement.Base.z
            sv.Visibility = False
            sv.recompute()
            group.addObject(sv)

            wire = Part.Wire(sv.Shape.Edges)
            if not wire.isClosed():
                # Sometimes the wire is not closed because the edges are
                # not sorted and do not form a "chain". Therefore, sort them,
                # recreate the wire while also rounding the precision of the 
                # Vertices in order to avoid not closing because the points
                # are not close enougth
                wire = Part.Wire(Part.__sortEdges__(self._round(sv.Shape.Edges)))
                if not wire.isClosed():
                    _wrn(f"Projected Face for {object.Label} does not produce a closed wire. Not adding to slab construction ...")
                    continue

            face = Part.Face(wire)
            extrude = face.extrude(-Z_NORM*floor.floorThickness.Value)
            part = Part.show(extrude, "Footprint")
            part.Label = f"Extrude-{floor.Label}-{object.Label}-footprint"
            part.recompute()
            part.Visibility = False
            part.ViewObject.ShowInTree = False
            objects_to_fuse.append(part)
        return objects_to_fuse

    def _round(self, edges, decimals=2):
        """
        Rounds the coordinates of all vertices in a list of edges to the specified number of decimals.

        :param edges: A list of Part.Edge objects.
        :param decimals: Number of decimal places to round to (default: 2).
        :return: A list of edges with rounded vertices.
        """
        new_edges = []

        for edge in edges:
            vertices = edge.Vertexes
            if len(vertices) != 2:  # Line or similar
                raise ValueError("Unsupported edge type: Only straight edges are handled.")
            new_vertices = [
                App.Vector(round(v.X, decimals), round(v.Y, decimals), round(v.Z, decimals))
                for v in vertices
            ]
            # Create a new edge with the rounded vertices
            new_edge = Part.Edge(Part.LineSegment(new_vertices[0], new_vertices[1]))
            new_edges.append(new_edge)
        return new_edges


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

        space = face = None
        if self.importer.preferences["MERGE"]:
            space = self.get_fc_object(elm.get("id"), 'room')

        # A Room is composed of a space with a Face as the base object
        if not space:
            floor_z = dim_fc2sh(floor.Placement.Base.z)
            points = [ coord_sh2fc(App.Vector(float(p.get('x')), float(p.get('y')), floor_z)) for p in elm.findall('point') ]
            # remove consecutive identical points
            points = [points[i] for i in range(len(points)) if i == 0 or points[i] != points[i - 1]]

            # Create a reference face that can be used later on to create
            # the floor & ceiling decoration...
            reference_face = Draft.make_wire(points, closed=True, face=True, support=None)
            reference_face.Label = elm.get('name', 'Room') + '-reference'
            reference_face.Visibility = False
            reference_face.recompute()
            floor.getObject(floor.ReferenceFacesGroupName).addObject(reference_face)

            # NOTE: for room to properly display and calculate the area, the 
            # Base object can not be a face but must have a height...
            footprint = App.ActiveDocument.addObject("Part::Feature", "Footprint")
            footprint.Shape = reference_face.Shape.extrude(Z_NORM)
            footprint.Label = elm.get('name', 'Room') + '-footprint'

            space = Arch.makeSpace(footprint)
            space.IfcType = "Space"
            space.Label = elm.get('name', 'Room')
            self._set_properties(space, elm)

            space.setExpression('ElevationWithFlooring', f"{footprint.Name}.Shape.BoundBox.ZMin")
            self.setp(space, "App::PropertyLink", "ReferenceFace", "The Reference Part.Face", reference_face)
            self.setp(space, "App::PropertyString", "ReferenceFloorName", "The name of the Arch.Floor this room belongs to", floor.Name)

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
        assert floor != None, f"Missing floor '{level_id}' for <wall> '{elm.get('id')}' ..."

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
        wall.Base.Label = f"wall{i}-wallshape"
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
        # Since their placement and other characteristics are dependant of
        # the wall elements to be created (such as Door&Windows), their
        # creation is delayed until the
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
        sweep = self._make_sweep(section_start, section_end, spine)
        # Sometimes the Part::Sweep creates a "twisted" sweep which
        # result in a broken wall. The solution is to use a compound
        # object based on ruled surface instead.
        # See https://github.com/FreeCAD/FreeCAD/issues/18658 and related OCCT
        #   ticket
        if sweep.Shape.isNull() or not sweep.Shape.isValid():
            if is_wall_straight:
                _log(f"Sweep's shape is invalid, using ruled surface instead ...")
                App.ActiveDocument.removeObject(sweep.Label)
                compound_solid, base_object = self._make_compound(section_start, section_end, spine)
                wall = Arch.makeWall(compound_solid)
            else:
                _wrn(f"Sweep's shape is invalid, but mitigation is not available!")
                wall = Arch.makeWall(sweep)
        else:
            wall = Arch.makeWall(sweep)

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
        App.ActiveDocument.recompute([section_start, section_end, spine])
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
        App.ActiveDocument.recompute([section_start, section_end, spine])
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

        section_start = self._get_section(wall_details, True, prev_wall_details)
        section_end = self._get_section(wall_details, False, next_wall_details)

        spine = Draft.makeLine(start, end)
        spine.Label = f"Spine"
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
        length = 0
        if invert_angle:
            spine = Draft.makeCircle(radius, placement, False, a1, a2)
            length = abs(radius * math.radians(a2 - a1))
        else:
            spine = Draft.makeCircle(radius, placement, False, a2, a1)
            length = abs(radius * math.radians(a1 - a2))

        # The Length property is used in the Wall to calculate volume, etc...
        # Since make Circle does not calculate this Length I calculate it here...
        self.importer.set_property(spine, "App::PropertyFloat", "Length", "The length of the Arc", length, group="Draft")
        # The Start and End property are used in the Wall to  determine Facebinders 
        # characteristics...
        self.importer.set_property(spine, "App::PropertyVector", "Start", "The start point of the Arc", start, group="Draft")
        self.importer.set_property(spine, "App::PropertyVector", "End", "The end point of the Arc", end, group="Draft")

        spine.Label = f"Spine"
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
            section = Draft.makeRectangle([i_start, i_end, i_end_z, i_start_z], face=True)
            if self.importer.preferences["DEBUG"]:
                _log(f"section: {section}")
        else:
            (start, end, thickness, height_start, height_end, _) = wall_details
            height = height_start if at_start else height_end
            center = start if at_start else end
            a1, a2, _ = self._get_normal_angles(wall_details)
            z_rotation = a1 if at_start else a2
            section = Draft.makeRectangle(thickness, height, face=True)
            Draft.move([section], App.Vector(-thickness/2, 0, 0))
            Draft.rotate([section], 90, ORIGIN, X_NORM)
            Draft.rotate([section], z_rotation, ORIGIN, Z_NORM)
            Draft.move([section], center)

        if self.importer.preferences["DEBUG"]:
            section.recompute()
            _color_section(section)

        section.Label = "Section-start" if at_start else "Section-end"
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
            if np.sign(arc_extent) != np.sign(DraftVecUtils.angle(start-center, end-center, Z_NORM)):
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

    def post_process(self, obj):
        if self.importer.preferences["DECORATE_SURFACES"]:
            floor = App.ActiveDocument.getObject(obj.ReferenceFloorName)

            (left_face_name, left_face, right_face_name, right_face) = self._get_faces(obj)

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
                self._create_baseboard(floor, wall, side, face)

    def _create_baseboard(self, floor, wall, side, face):

        baseboard_width = getattr(wall, f"{side}Thickness").Value
        baseboard_height = getattr(wall, f"{side}Height").Value

        # Once I have the face, I get the lowest edge.
        lowest_z = float('inf')
        bottom_edge = None

        for edge in face.Edges:
            if edge and edge.CenterOfMass and edge.CenterOfMass.z < lowest_z:
                lowest_z = edge.CenterOfMass.z
                bottom_edge = edge

        p_normal = face.normalAt(bottom_edge.CenterOfMass.x, bottom_edge.CenterOfMass.y)
        p_normal.z = 0
        offset_vector = p_normal.normalize().multiply(baseboard_width)
        offset_bottom_edge = bottom_edge.translated(offset_vector)

        if self.importer.preferences["DEBUG"]:
            _log(f"Creating {side} for {wall.Label} from edge {self._pe(bottom_edge, True)} to {self._pe(offset_bottom_edge, True)} (normal={self._pv(p_normal, True, 4)})")

        edge0 = bottom_edge.copy()
        edge1 = Part.makeLine(bottom_edge.Vertexes[1].Point, offset_bottom_edge.Vertexes[1].Point)
        edge2 = offset_bottom_edge
        edge3 = Part.makeLine(offset_bottom_edge.Vertexes[0].Point, bottom_edge.Vertexes[0].Point)

        # make sure all edges are coplanar...
        ref_z = bottom_edge.CenterOfMass.z
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

    def _get_faces(self, wall):
        """Returns the name of the left and right face for `wall`

        The face names are suitable for selection later on when creating 
        the Facebinders and baseboards. Note, that this must be executed
        once the wall has been completly been constructued. If a window
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
        # start to end) that pass throuh the center of gravity of the wall
        # Hopefully the COG of the face will always be on the correct side
        # of the COG of the wall
        wall_start = wall.BaseObjects[2].Start
        wall_end = wall.BaseObjects[2].End
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
        # window. In FC the placement is defined on the face of the wall that
        # contains the windows. The makes this calculation rather cumbersome.
        x_center = float(elm.get('x'))
        y_center = float(elm.get('y'))
        z_center = float(elm.get('elevation', 0))

        # This is the FC coordinate of the center point of the lower face of the
        # window. This then needs to be moved to the proper face on the wall and
        # offset properly with respect to the wall's face.
        center = coord_sh2fc(App.Vector(x_center, y_center, z_center))
        center.z += floor.Placement.Base.z

        # First create a solid representing the window countour and find the
        # walls containing that window
        width = dim_sh2fc(elm.get('width'))
        depth = dim_sh2fc(elm.get('depth'))
        height = dim_sh2fc(elm.get('height'))
        angle = float(elm.get('angle', 0))

        corner = center.add(App.Vector(-width/2, -depth/2, -height/2))

        # Then create a box that represent the BoundingBox of the windows
        # to find out which wall contains the window.
        solid = Part.makeBox(width, depth, height)
        solid.rotate(solid.CenterOfMass, Z_NORM, math.degrees(ang_sh2fc(angle)))
        solid.translate(corner)

        # Get all the walls hosting that door/window...
        wall_width = -DEFAULT_WALL_WIDTH
        walls = self._get_containing_walls(floor, solid)
        if len(walls) == 0:
            _err(f"Missing wall for <doorOrWindow> {elm.get('id')}. Defaulting to width {DEFAULT_WALL_WIDTH} ...")
        else:
            # NOTE:
            # The main host (the one defining the width of the door/window) is
            # the one that contains the CenterOfMass of the windows, or maybe
            # the one that has the same normal?
            wall_width = float(walls[0].Width)
            com = solid.CenterOfMass
            for wall in walls:
                if wall.Shape.isInside(com, 1, False):
                    wall_width = float(wall.Width)

        center2corner = App.Vector(-width/2, -wall_width/2, 0)
        rotation = App.Rotation(Z_NORM, math.degrees(ang_sh2fc(angle)))
        center2corner = rotation.multVec(center2corner)
        corner = center.add(center2corner)

        pl = App.Placement(
            corner,  # translation
            App.Rotation(math.degrees(ang_sh2fc(angle)), 0, 90),  # rotation
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

        # See the https://wiki.freecad.org/Arch_Window for details about these values
        # Only using Opening / Fixed / Simple Door
        h1 = min(50,height*.025) # 2.5% of frame
        h2 = h1
        h3 = 0
        w1 = wall_width
        w2 = min(20.0,wall_width*.2) # 20% of width
        o1 = 0
        o2 = (wall_width-w2)/2
        window = Arch.makeWindowPreset(windowtype, width, height, h1, h2, h3, w1, w2, o1, o2, pl)
        window.IfcType = ifc_type

        mirrored = bool(elm.get('modelMirrored', False))
        if ifc_type == 'Door' and mirrored:
            window.OperationType = "SINGLE_SWING_RIGHT"

        # Adjust symbol plan, Sweet Home has the opening in the opposite side by default
        window.ViewObject.Proxy.invertOpening()
        if mirrored:
            window.ViewObject.Proxy.invertHinge()

        window.Hosts = walls
        return window

    def _get_containing_walls(self, floor, solid):
        """Returns the wall(s) intersecting with the door/window.

        Args:
            floor (Arch.Level): the level the solid must belongs to
            solid (Part.Solid): the solid to test against each wall's
                bounding box

        Returns:
            list(Arch::Wall): the wall(s) containing the given solid
        """
        host_walls = []
        for wall in self.importer.get_walls(floor):
            if solid.common(wall.Shape).Volume > 0:
                host_walls.append(wall)
        return host_walls


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
            feature = self._create_equipment(floor, elm)

        color = elm.get('color', self.importer.preferences["DEFAULT_FLOOR_COLOR"])
        set_color_and_transparency(feature, color)

        self.setp(feature, "App::PropertyString", "shType", "The element type", 'pieceOfFurniture')
        self.set_furniture_common_properties(feature, elm)
        self.set_piece_of_furniture_common_properties(feature, elm)
        self.set_piece_of_furniture_horizontal_rotation_properties(feature, elm)
        self.setp(feature, "App::PropertyString", "id", "The furniture's id", furniture_id)

        if 'FurnitureGroupName' not in floor.PropertiesList:
            group = floor.newObject("App::DocumentObjectGroup", "Furnitures")
            self.setp(floor, "App::PropertyString", "FurnitureGroupName", "The DocumentObjectGroup name for all furnitures on this floor", group.Name)

        if self.importer.preferences["CREATE_ARCH_EQUIPMENT"]:
            p = feature.Shape.BoundBox.Center
        else:
            p = feature.Mesh.BoundBox.Center

        space = self.get_space(floor, p)
        if space:
            space.Group = space.Group + [feature]
        else:
            _log(f"No space found to enclose {feature.Label}. Adding to generic group.")
            floor.getObject(floor.FurnitureGroupName).addObject(feature)

        # We add the object to the list of known object that can then
        # be referenced elsewhere in the SH3D model (i.e. lights).
        self.importer.add_fc_objects(feature)

    def _create_equipment(self, floor, elm):
        width = dim_sh2fc(float(elm.get('width')))
        depth = dim_sh2fc(float(elm.get('depth')))
        height = dim_sh2fc(float(elm.get('height')))
        x = float(elm.get('x', 0))
        y = float(elm.get('y', 0))
        z = float(elm.get('elevation', 0.0))
        height_in_plan = elm.get('heightInPlan', 0.0)
        pitch = float(elm.get('pitch', 0.0))  # X SH3D Axis
        roll = float(elm.get('roll', 0.0))    # Y SH3D Axis
        angle = float(elm.get('angle', 0.0))  # Z SH3D Axis
        name = elm.get('name')
        model_rotation = elm.get('modelRotation', None)
        mirrored = bool(elm.get('modelMirrored', "false") == "true")

        # The meshes are normalized, centered, facing up.
        # Center, Scale, X Rotation && Z Rotation (in FC axes), Move
        mesh = self._get_mesh(elm)
        bb = mesh.BoundBox
        transform = App.Matrix()
        # In FC the reference is the "upper left" corner
        transform.move(-bb.Center)
        if model_rotation:
            rij = [ float(v) for v in model_rotation.split() ]
            rotation = App.Rotation(
                App.Vector(rij[0], rij[1], rij[2]),
                App.Vector(rij[3], rij[4], rij[5]),
                App.Vector(rij[6], rij[7], rij[8])
                )
            _msg(f"{elm.get('id')}: modelRotation is not yet implemented ...")
        transform.scale(width/bb.XLength, height/bb.YLength, depth/bb.ZLength)
        # NOTE: the model is facing up, thus y and z are inverted
        transform.rotateX(math.pi/2)
        transform.rotateX(-pitch)
        transform.rotateY(roll)
        transform.rotateZ(ang_sh2fc(angle))

        mesh.transform(transform)

        if self.importer.preferences["CREATE_ARCH_EQUIPMENT"]:
            shape = Part.Shape()
            shape.makeShapeFromMesh(mesh.Topology, 1)
            equipment = Arch.makeEquipment(name=name)
            equipment.Shape = shape
        else:
            equipment = App.ActiveDocument.addObject("Mesh::Feature", name)
            equipment.Mesh = mesh

        equipment.Placement.Base = coord_sh2fc(App.Vector(x, y, z))
        equipment.Placement.Base.z += floor.Placement.Base.z
        equipment.Placement.Base.z += mesh.BoundBox.ZLength / 2

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


def ang_sh2fc(angle:float):
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
    # TODO: it seems a shininess of 0 means the wall loose its
    # color. We leave it at the default setting untill a later time
    return
    if not App.GuiUp or not shininess:
        return
    if hasattr(obj.ViewObject, "ShapeAppearance"):
        mat = obj.ViewObject.ShapeAppearance[0]
        mat.Shininess = float(shininess)/100
        obj.ViewObject.ShapeAppearance = mat


def percent_sh2fc(percent):
    # percent goes from 0 -> 1 in SH3d and 0 -> 100 in FC
    return int(float(percent)*100)
