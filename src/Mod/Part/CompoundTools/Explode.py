import FreeCAD
import Part

from .CompoundFilter import makeCompoundFilter

def explodeCompound(compound_obj, b_group = None):
    """explodeCompound(compound_obj, b_group = None): creates a bunch of compound filters, to extract every child of a compound into a separate object.
    group: if True, Group is always made. If False, group is never made. If None, group is made if there is more than one child.
    returns: (group_object, list_of_child_objects)"""

    if (isinstance(compound_obj, FreeCAD.GeoFeature) and
        isinstance(compound_obj.getPropertyOfGeometry(), Part.Shape)):
        sh = compound_obj.getPropertyOfGeometry()
    else:
        raise TypeError("Object must be App.GeoFeature with Part.Shape property")

    n = len(sh.childShapes(False,False))
    if b_group is None:
        b_group = n > 1
    if b_group:
        group = compound_obj.Document.addObject('App::DocumentObjectGroup','GrExplode_'+compound_obj.Name)
        group.Label = 'Exploded {obj.Label}'.format(obj = compound_obj)
    else:
        group = compound_obj.Document
    features_created = []
    for i in range(0, n):
        cf = makeCompoundFilter('{obj.Name}_child{child_num}'.format(obj = compound_obj, child_num = i), group)
        cf.Label = '{obj.Label}.{child_num}'.format(obj = compound_obj, child_num = i)
        cf.Base = compound_obj
        cf.FilterType = 'specific items'
        cf.items = str(i)
        if cf.ViewObject is not None:
            cf.ViewObject.DontUnhideOnDelete = True
        features_created.append(cf)
    return (group, features_created)

