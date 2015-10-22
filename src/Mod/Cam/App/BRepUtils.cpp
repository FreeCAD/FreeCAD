#include "BRepUtils.h"

#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>



double BRepUtils::GetLength(const TopoDS_Edge& edge)
{
	GProp_GProps lProps;
	BRepGProp::LinearProperties(edge,lProps);
	return lProps.Mass();
}

bool BRepUtils::CheckTopologie(const TopoDS_Shape& shape)
{
	TopTools_IndexedDataMapOfShapeListOfShape aMap;
	aMap.Clear();
	TopExp::MapShapesAndAncestors(shape,TopAbs_EDGE,TopAbs_FACE,aMap);
	TopExp_Explorer anExplorer;
	for(anExplorer.Init(shape,TopAbs_EDGE);anExplorer.More();anExplorer.Next())
	{
		const TopTools_ListOfShape& aFaceList = aMap.FindFromKey(anExplorer.Current());
		
		TopTools_ListIteratorOfListOfShape aListIterator(aFaceList);
		int i=0;
		for(aListIterator.Initialize(aFaceList);aListIterator.More();aListIterator.Next())
		{
			i++;
		}
		if(i<2) 
		{
			cout << "less" << endl;
		}
	}
	return true;
}
