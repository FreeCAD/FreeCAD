#include "TopoShapeMapper.h"

namespace Part
{

void ShapeMapper::expand(const TopoDS_Shape &d, std::vector<TopoDS_Shape> &shapes)
{
    if (d.IsNull()) return;
    for(TopExp_Explorer xp(d, TopAbs_FACE);xp.More();xp.Next())
        shapes.push_back(xp.Current());
    for(TopExp_Explorer xp(d, TopAbs_EDGE, TopAbs_FACE);xp.More();xp.Next())
        shapes.push_back(xp.Current());
    for(TopExp_Explorer xp(d, TopAbs_VERTEX, TopAbs_EDGE);xp.More();xp.Next())
        shapes.push_back(xp.Current());
}

void ShapeMapper::populate(bool generated,
                           const TopTools_ListOfShape &src,
                           const TopTools_ListOfShape &dst)
{
    for(TopTools_ListIteratorOfListOfShape it(src);it.More();it.Next())
        populate(generated, it.Value(), dst);
}

void ShapeMapper::populate(bool generated,
                           const TopoShape &src,
                           const TopTools_ListOfShape &dst)
{
    if(src.isNull())
        return;
    std::vector<TopoDS_Shape> dstShapes;
    for(TopTools_ListIteratorOfListOfShape it(dst);it.More();it.Next())
        expand(it.Value(), dstShapes);
    insert(generated, src.getShape(), dstShapes);
}

void ShapeMapper::insert(bool generated, const TopoDS_Shape &s, const TopoDS_Shape &d)
{
    if (s.IsNull() || d.IsNull()) return;
    // Prevent an element shape from being both generated and modified
    if (generated) {
        if (_modifiedShapes.count(d))
            return;
        _generatedShapes.insert(d);
    } else {
        if( _generatedShapes.count(d))
            return;
        _modifiedShapes.insert(d);
    }
    auto &entry = generated?_generated[s]:_modified[s];
    if(entry.shapeSet.insert(d).second)
        entry.shapes.push_back(d);
};

void ShapeMapper::insert(bool generated, const TopoDS_Shape &s, const std::vector<TopoDS_Shape> &d)
{
    if (s.IsNull() || d.empty()) return;
    auto &entry = generated?_generated[s]:_modified[s];
    for(auto &shape : d) {
        // Prevent an element shape from being both generated and modified
        if (generated) {
            if (_modifiedShapes.count(shape))
                continue;
            _generatedShapes.insert(shape);
        } else {
            if( _generatedShapes.count(shape))
                continue;
            _modifiedShapes.insert(shape);
        }
        if(entry.shapeSet.insert(shape).second)
            entry.shapes.push_back(shape);
    }
};

}
