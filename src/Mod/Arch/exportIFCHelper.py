import FreeCAD, Draft, json, ifcopenshell

def getObjectsOfIfcType(objects, ifcType):
    results = []
    for object in objects:
        if object.IfcType == ifcType:
            results.append(object)
    return results

class ContextCreator:
    def __init__(self, file, objects):
        self.file = file
        self.objects = objects
        self.project_object = self.getProjectObject()
        self.project_data = self.getProjectObjectData()
        self.model_context = self.createGeometricRepresentationContext()
        self.model_view_subcontext = self.createGeometricRepresentationSubContext()
        self.target_crs = self.createTargetCRS()
        self.map_conversion = self.createMapConversion()
        self.project = self.createProject()

    def createGeometricRepresentationContext(self):
        return self.file.createIfcGeometricRepresentationContext(
            None, "Model",
            3, 1.0E-05,
            self.file.by_type("IfcAxis2Placement3D")[0],
            self.createTrueNorth())

    def createGeometricRepresentationSubContext(self):
        return self.file.createIfcGeometricRepresentationSubContext(
            "Body", "Model",
            None, None, None, None,
            self.model_context, None, "MODEL_VIEW", None)

    def createTargetCRS(self):
        try:
            return self.file.createIfcCoordinateReferenceSystem(
                self.project_data["name"],
                self.project_data["description"],
                self.project_data["geodetic_datum"],
                self.project_data["vertical_datum"]
                )
        except:
            return None

    def createMapConversion(self):
        try:
            return self.file.createIfcMapConversion(
                self.model_context, self.target_crs,
                float(self.project_data["eastings"]),
                float(self.project_data["northings"]),
                float(self.project_data["orthogonal_height"]),
                float(self.project_data["x_axis_abscissa"]),
                float(self.project_data["x_axis_ordinate"]),
                float(self.project_data["scale"])
                )
        except:
            return None

    def createTrueNorth(self):
        # TODO: don't hardcode this value
        return self.file.createIfcDirection((0., 1., 0.))

    def createProject(self):
        if not self.project_object:
            return self.createAutomaticProject()
        return self.createCustomProject()

    def createAutomaticProject(self):
        return self.file.createIfcProject(
            self.getProjectGUID(),
            self.file.by_type("IfcOwnerHistory")[0],
            FreeCAD.ActiveDocument.Name, None,
            None, None, None, [self.model_context],
            self.file.by_type("IfcUnitAssignment")[0])

    def createCustomProject(self):
        return self.file.createIfcProject(
            self.getProjectGUID(),
            self.file.by_type("IfcOwnerHistory")[0],
            self.project_object.Label, self.project_object.Description,
            self.project_object.ObjectType, self.project_object.LongName,
            self.project_object.Phase,
            [self.model_context],
            self.file.by_type("IfcUnitAssignment")[0])

    def getProjectGUID(self):
        # TODO: Do not generate a new one each time, but at least this one
        # conforms to the community consensus on how a GUID is generated.
        return ifcopenshell.guid.new()

    def getProjectObject(self):
        try:
            return getObjectsOfIfcType(self.objects, "Project")[0]
        except:
            return None

    def getProjectObjectData(self):
        if not self.project_object:
            return {}
        return json.loads(self.project_object.IfcData['complex_attributes'])["RepresentationContexts"]
