from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase

@export(
    Father="PyObjectBase",
    Name="QuickMeasurePy",
    Twin="QuickMeasure",
    TwinPointer="QuickMeasure",
    Include="Mod/Measure/Gui/QuickMeasure.h",
    Namespace="MeasureGui",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class QuickMeasurePy(PyObjectBase):
    """
    Selection Observer for the QuickMeasure label.
    """
