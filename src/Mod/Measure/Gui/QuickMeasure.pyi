from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase

@export(
    Include="Mod/Measure/Gui/QuickMeasure.h",
    Namespace="MeasureGui",
    Constructor=True,
    Delete=True,
)
class QuickMeasure(PyObjectBase):
    """
    Selection Observer for the QuickMeasure label.
    """
