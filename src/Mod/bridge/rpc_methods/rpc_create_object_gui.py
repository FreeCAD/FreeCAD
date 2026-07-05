from __future__ import annotations

from rpc import *


@rpc_method
def _create_object_gui(self, doc_name, obj: _Object):
    import ObjectsFem

    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        FreeCAD.Console.PrintError(f"Document '{doc_name}' not found.\n")
        return f"Document '{doc_name}' not found.\n"
    try:
        if obj.type == "Fem::FemMeshGmsh" and obj.analysis:
            from femmesh.gmshtools import GmshTools

            res = getattr(doc, obj.analysis).addObject(
                ObjectsFem.makeMeshGmsh(doc, obj.name)
            )[0]
            if "Part" in obj.properties:
                target_obj = doc.getObject(obj.properties["Part"])
                if target_obj:
                    res.Part = target_obj
                else:
                    raise ValueError(
                        f"Referenced object '{obj.properties['Part']}' not found."
                    )
                del obj.properties["Part"]
            else:
                raise ValueError("'Part' property not found in properties.")
            for param, value in obj.properties.items():
                if hasattr(res, param):
                    setattr(res, param, value)
            doc.recompute()
            gmsh_tools = GmshTools(res)
            gmsh_tools.create_mesh()
            FreeCAD.Console.PrintMessage(
                f"FEM Mesh '{res.Name}' generated successfully in '{doc_name}'.\n"
            )
        elif obj.type.startswith("Fem::"):
            fem_make_methods = {
                "MaterialCommon": ObjectsFem.makeMaterialSolid,
                "AnalysisPython": ObjectsFem.makeAnalysis,
            }
            obj_type_short = obj.type.split("::")[1]
            method_name = "make" + obj_type_short
            make_method = fem_make_methods.get(
                obj_type_short, getattr(ObjectsFem, method_name, None)
            )
            if callable(make_method):
                res = make_method(doc, obj.name)
                _set_object_property(doc, res, obj.properties)
                FreeCAD.Console.PrintMessage(
                    f"FEM object '{res.Name}' created with '{method_name}'.\n"
                )
            else:
                raise ValueError(
                    f"No creation method '{method_name}' found in ObjectsFem."
                )
            if obj.type != "Fem::AnalysisPython" and obj.analysis:
                getattr(doc, obj.analysis).addObject(res)
        else:
            res = doc.addObject(obj.type, obj.name)
            _set_object_property(doc, res, obj.properties)
            FreeCAD.Console.PrintMessage(
                f"{res.TypeId} '{res.Name}' added to '{doc_name}' via RPC.\n"
            )
        doc.recompute()
        return True
    except Exception as e:
        return str(e)
