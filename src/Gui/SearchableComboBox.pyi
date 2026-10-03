from Base.Metadata import export, constmethod, no_args
from PySide6 import QComboBoxPy

@export(
    Constructor=True,
    Delete=True,
    Father="QComboBoxPy",
    Name="SearchableComboBoxPy",
    Twin="SearchableComboBox",
    TwinPointer="SearchableComboBox",
    Include="Gui/SearchableComboBox.h",
    Namespace="Gui",
    FatherInclude="QComboBox",
)
class SearchableComboBoxPy(QComboBoxPy):
    @property
    def searchable(self) -> bool: ...
    @searchable.setter
    def searchable(self, value: bool) -> None: ...
    @property
    def grid(self) -> bool: ...
    @grid.setter
    def grid(self, value: bool) -> None: ...
    @property
    def popupMaximumHeight(self) -> int: ...
    @popupMaximumHeight.setter
    def popupMaximumHeight(self, value: int) -> None: ...
    @property
    def gridFixedColumns(self) -> int: ...
    @gridFixedColumns.setter
    def gridFixedColumns(self, value: int) -> None: ...
    @property
    def popupScrollBar(self) -> bool: ...
    @popupScrollBar.setter
    def popupScrollBar(self, value: bool) -> None: ...
