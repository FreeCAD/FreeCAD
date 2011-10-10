#ifndef _BRepUtils_HeaderFile
#define _BRepUtils_HeaderFile

#ifndef _Standard_Macro_HeaderFile
#include <Standard_Macro.hxx>
#endif

class TopoDS_Edge;
class TopoDS_Shape;

class BRepUtils
{
public:

	Standard_EXPORT	static bool CheckTopologie(const TopoDS_Shape& shape);
	Standard_EXPORT	static double GetLength(const TopoDS_Edge& edge);

};


#endif //_BRepUtils_HeaderFile

