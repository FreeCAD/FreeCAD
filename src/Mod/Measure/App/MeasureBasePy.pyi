from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Father="DocumentObjectPy",
    Name="MeasureBasePy",
    Twin="MeasureBase",
    TwinPointer="MeasureBase",
    Include="Mod/Measure/App/MeasureBase.h",
    Namespace="Measure",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
    Constructor=True,
)
class MeasureBasePy(DocumentObject):
    """
    User documentation here
    """
