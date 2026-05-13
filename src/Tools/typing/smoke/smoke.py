"""Smoke type-check target for the generated binding stubs.

This file is intentionally import-heavy and call-heavy. The generator's
``check`` mode points Pyright and Pyrefly at this module together with the
generated stub tree so both checkers exercise a representative slice of the
public FreeCAD Python API.

Keep this file focused on broad API coverage rather than runtime behavior:
- imports should make it obvious which public modules must resolve
- calls should touch overloads and enum-like values that are easy to regress
- values are assigned only to force the type checker to validate signatures

The code is never executed as part of the smoke check. It exists only so static
type checkers can walk the generated stubs and complain when a symbol moves,
disappears, or gains an incompatible signature.
"""

from __future__ import annotations

from typing import Any, Literal, assert_type, cast, reveal_type

import FreeCAD
import FreeCAD.Console as Console
import FreeCAD.Qt as Qt
import FreeCAD.Units as Units
import FreeCADGui
import FreeCADGui.Selection as GuiSelection
import Materials
import PartDesignGui
import PathApp
import QtUnitGui
import Sandbox
import SpreadsheetGui
import TechDrawGui
from FreeCAD import DocumentObject
from FreeCAD.Base import (
    Axis,
    BoundBox,
    Matrix,
    Placement,
    Precision,
    ProgressIndicator,
    Quantity,
    Rotation,
    Vector,
)
import Part
from Part import Edge, Shape, Wire
from Part.Geom2d import Circle2d
from Part.HLRBRep import HLRToShape


class SelectionGate:
    """Minimal protocol object used to exercise ``Selection.addSelectionGate``."""

    def allow(self, doc: object, obj: DocumentObject, sub: str, /) -> bool:
        return bool(doc or obj or sub)


class ParameterObserver:
    """Minimal observer used to exercise ``_ParameterGrp.Attach``."""

    def onChange(
        self,
        group: FreeCAD._ParameterGrp,
        param_type: str,
        name: str,
        value: str,
        /,
    ) -> None:
        _ = (group, param_type, name, value)


class ParameterManagerObserver:
    """Minimal observer used to exercise ``_ParameterGrp.AttachManager``."""

    def slotParamChanged(
        self,
        group: FreeCAD._ParameterGrp,
        param_type: str,
        name: str,
        value: str,
        /,
    ) -> None:
        _ = (group, param_type, name, value)


def exercise(
    obj: DocumentObject,
    shape: Shape,
    edge: Edge,
    wire: Wire,
    vector: Vector,
    matrix: Matrix,
    placement: Placement,
    rotation: Rotation,
    circle: Circle2d,
    hlr: HLRToShape,
) -> None:
    """Touch a broad set of generated stub surfaces in one checker entrypoint."""

    document = FreeCAD.activeDocument()
    active_document = FreeCAD.ActiveDocument
    documents = FreeCAD.listDocuments()
    part_feature = cast(FreeCAD.Document, object()).addObject("Part::Feature", "Shape")
    copied_object = cast(FreeCAD.Document, object()).copyObject(obj, True)
    transaction = FreeCAD.getActiveTransaction()
    console_status = Console.GetStatus("Console", "Log")
    console_observers = Console.GetObservers()
    gui_up = FreeCAD.GuiUp
    parsed_quantity = Units.parseQuantity("10 mm")
    schema = Units.getSchema()
    schema_names = Units.listSchemas()
    schema_description = Units.listSchemas(schema)
    schema_translation = Units.schemaTranslate(parsed_quantity, schema)
    quantity_number = Units.toNumber(parsed_quantity)
    fixed_number = Units.toNumber(1.0, "f", 2)
    translated = Qt.translate("Context", "Source")
    translated_with_disambiguation = Qt.translate("Context", "Source", None, -1)
    translate_noop = Qt.QT_TRANSLATE_NOOP("Context", "Source")
    translate_noop3 = Qt.QT_TRANSLATE_NOOP3("Context", "Source", "disambiguation")
    tr_noop = Qt.QT_TR_NOOP("Source")
    axis = FreeCAD.Axis()
    bound_box = FreeCAD.BoundBox()
    parameters = FreeCAD.ParamGet("User parameter:BaseApp")
    parameters.GetGroupName()
    child_parameters = parameters.GetGroup("Preferences")
    progress = ProgressIndicator()
    control = FreeCADGui.Control
    task_placement = FreeCADGui.TaskPlacement()
    ui_loader = FreeCADGui.UiLoader()
    gui_document = FreeCADGui.activeDocument()
    edit_document = FreeCADGui.editDocument()
    active_gui_document = FreeCADGui.ActiveDocument
    gui_document_by_name = FreeCADGui.getDocument("Document")
    gui_view = FreeCADGui.activeView()
    gui_main_window = FreeCADGui.getMainWindow()
    resource_from_dialog = FreeCADGui.createDialog("dialog.ui")
    single_viewer = FreeCADGui.createViewer()
    split_viewer = FreeCADGui.createViewer(2, "Split")
    active_workbench = FreeCADGui.activeWorkbench()
    workbench_by_name = FreeCADGui.getWorkbench("NoneWorkbench")
    workbenches = FreeCADGui.listWorkbenches()
    workbench = FreeCADGui.Workbench()
    user_input: FreeCADGui.UserInput = FreeCADGui.UserInput.MouseLeft
    input_hint = FreeCADGui.InputHint(
        "%1 pick",
        user_input,
        (FreeCADGui.UserInput.KeyControl, FreeCADGui.UserInput.KeyEnter),
    )
    resolve_mode: GuiSelection.ResolveMode = GuiSelection.ResolveMode.NoResolve
    selection_style_enum: GuiSelection.SelectionStyle = GuiSelection.SelectionStyle.NormalSelection
    main_window = cast(FreeCADGui._MainWindow, object())
    mdi_view = cast(FreeCADGui._MDIView, object())
    task_dialog = cast(FreeCADGui._TaskDialog, object())
    view = cast(FreeCADGui._View3DInventor, object())
    viewer = cast(FreeCADGui._View3DInventorViewer, object())
    resource = cast(FreeCADGui._PyResource, object())
    parameter_observer = ParameterObserver()
    parameter_manager_observer = ParameterManagerObserver()
    material = cast(Materials.Material, object())
    path_command = cast(PathApp.Command, object())
    part_design_view_provider = cast(PartDesignGui.ViewProvider, object())
    selection_filter = GuiSelection.Filter("SELECT Part::Feature")
    preselection = GuiSelection.getPreselection()
    selection = GuiSelection.getSelection()
    picked_selection = GuiSelection.getPickedList()
    complete_selection = GuiSelection.getCompleteSelection()
    selection_ex = GuiSelection.getSelectionEx()
    selection_object = GuiSelection.getSelectionObject("Document", "Object", "Edge1")
    stacked_selection = GuiSelection.getSelectionFromStack()
    protector = cast(Sandbox._DocumentProtector, object())
    unit_test = cast(QtUnitGui._UnitTest, object())
    sheet_view = cast(SpreadsheetGui._SheetView, object())
    page_view = cast(TechDrawGui._MDIViewPage, object())
    control.activeDialog()
    main_window.getWindows()
    view.getCamera()
    viewer.getSceneGraph()
    resource.value("objectName", "propertyName")
    GuiSelection.addSelection("Document", "Object")
    GuiSelection.addSelection(obj)
    GuiSelection.addSelection(obj, "Edge1", 1.0, 2.0, 3.0)
    GuiSelection.addSelection(obj, ["Edge1", "Edge2"])
    GuiSelection.updateSelection(True, obj)
    GuiSelection.removeSelection("Document", "Object")
    GuiSelection.removeSelection(obj, "Edge1")
    GuiSelection.clearSelection()
    GuiSelection.clearSelection("Document")
    GuiSelection.isSelected(obj)
    GuiSelection.setPreselection(obj, subname="Edge1", x=1.0, y=2.0, z=3.0, tp=1)
    GuiSelection.clearPreselection()
    GuiSelection.countObjectsOfType("Part::Feature")
    GuiSelection.enablePickedList(True)
    GuiSelection.setSelectionStyle(0)
    GuiSelection.setSelectionStyle(selection_style_enum)
    GuiSelection.addObserver(object())
    GuiSelection.removeObserver(object())
    GuiSelection.addSelectionGate("SELECT Part::Feature")
    GuiSelection.addSelectionGate(selection_filter)
    GuiSelection.addSelectionGate(SelectionGate())
    GuiSelection.removeSelectionGate()
    GuiSelection.setVisible(None)
    GuiSelection.pushSelStack()
    GuiSelection.hasSelection()
    GuiSelection.hasSubSelection()
    workbench.Initialize()
    workbench.ContextMenu("View")
    workbench.appendToolbar("Tools", ["CommandName"])
    workbench.removeToolbar("Tools")
    workbench.listToolbars()
    workbench.getToolbarItems()
    workbench.appendCommandbar("Commands", ["CommandName"])
    workbench.removeCommandbar("Commands")
    workbench.listCommandbars()
    workbench.appendMenu(["Root", "Child"], ["CommandName"])
    workbench.removeMenu(["Root", "Child"])
    workbench.listMenus()
    workbench.appendContextMenu("", "Separator")
    workbench.removeContextMenu("")
    workbench.reloadActive()
    workbench.name()
    workbench.GetClassName()
    FreeCADGui.listCommands()
    FreeCADGui.isCommandActive("CommandName")
    FreeCADGui.HintManager.show(input_hint)
    FreeCADGui.HintManager.hide()
    FreeCADGui.activateWorkbench("NoneWorkbench")
    FreeCADGui.addWorkbench(FreeCADGui.Workbench)
    FreeCADGui.removeWorkbench("WorkbenchName")
    FreeCADGui.addResourcePath("/tmp")
    FreeCADGui.addLanguagePath("/tmp")
    FreeCADGui.addIconPath("/tmp")
    FreeCADGui.addIcon("example", b"data", "XPM")
    FreeCADGui.getIcon("example")
    FreeCADGui.isIconCached("example")
    FreeCADGui.updateGui()
    FreeCADGui.updateLocale()
    FreeCADGui.setLocale("en")
    FreeCADGui.addPreferencePage("preferences.ui", "General")
    FreeCADGui.addPreferencePage(object, "General")
    FreeCADGui.addCommand("CommandName", object())
    FreeCADGui.runCommand("CommandName")
    FreeCADGui.SendMsgToActiveView("ViewFit")
    FreeCADGui.sendMsgToFocusView("ViewFit", True)
    FreeCADGui.hide("Object")
    FreeCADGui.show("Object")
    FreeCADGui.hideObject(obj)
    FreeCADGui.showObject(obj)
    FreeCADGui.open("macro.py")
    FreeCADGui.insert(b"model.iv")
    FreeCADGui.export([obj], bytearray(b"scene.iv"))
    FreeCADGui.setActiveDocument("Document")
    FreeCADGui.setActiveDocument(cast(FreeCAD.Document, object()))
    FreeCADGui.activeView("Gui::View3DInventor")
    FreeCADGui.activateView("Gui::View3DInventor", False)
    gui_document_by_name.activeView()
    gui_document_by_name.mdiViewsOfType("Gui::View3DInventor")
    FreeCADGui.getDocument(cast(FreeCAD.Document, object()))
    FreeCADGui.doCommand("pass")
    FreeCADGui.doCommandGui("pass")
    FreeCADGui.doCommandSkip("pass")
    FreeCADGui.addModule("FreeCAD")
    FreeCADGui.showDownloads()
    FreeCADGui.showPreferences()
    FreeCADGui.showPreferences("General", 0)
    FreeCADGui.showPreferencesByName("General")
    FreeCADGui.getMarkerIndex("circle")
    FreeCADGui.addDocumentObserver(object())
    FreeCADGui.removeDocumentObserver(object())
    FreeCADGui.addWorkbenchManipulator(object())
    FreeCADGui.removeWorkbenchManipulator(object())
    FreeCADGui.setUserEditMode("Default")
    FreeCADGui.loadFile("model.step")
    FreeCADGui.loadFile("model.step", "Import")
    FreeCADGui.coinRemoveAllChildren(object())
    FreeCADGui.suspendWaitCursor()
    FreeCADGui.resumeWaitCursor()
    FreeCADGui.showMainWindow()
    FreeCADGui.showMainWindow(True)
    FreeCADGui.exec_loop()
    FreeCADGui.setupWithoutGUI()
    FreeCADGui.embedToWindow("0x0")
    FreeCADGui.subgraphFromObject(obj)
    FreeCADGui.exportSubgraph(object(), object())
    FreeCAD.Console.PrintLog("message")
    material.Name
    path_command.toGCode()
    part_design_view_provider.setBodyMode(True)
    Console.PrintMessage("message")
    Console.PrintMessage("notifier", "message")
    Console.PrintLog("message")
    Console.PrintError("message")
    Console.PrintDeveloperError("message")
    Console.PrintUserError("message")
    Console.PrintTranslatedUserError("message")
    Console.PrintWarning("message")
    Console.PrintDeveloperWarning("message")
    Console.PrintUserWarning("message")
    Console.PrintTranslatedUserWarning("message")
    Console.PrintCritical("message")
    Console.PrintNotification("message")
    Console.PrintTranslatedNotification("message")
    Console.SetStatus("Console", "Log", True)
    Units.setSchema(schema)
    Qt.installTranslator("translations.qm")
    Qt.removeTranslators()
    parameters.SetBool("flag", 1)
    parameters.SetInt("number", 1)
    parameters.SetUnsigned("unsigned", 1)
    parameters.SetFloat("ratio", 1.0)
    parameters.SetString("name", "value")
    parameters.RemBool("flag")
    parameters.RemInt("number")
    parameters.RemUnsigned("unsigned")
    parameters.RemFloat("ratio")
    parameters.RemString("name")
    parameters.RemGroup("missing")
    parameters.RenameGroup("old", "new")
    parameters.CopyTo(child_parameters)
    parameters.Attach(parameter_observer)
    parameters.AttachManager(parameter_manager_observer)
    parameters.Detach(parameter_observer)
    parameters.Notify("name")
    parameters.NotifyAll()
    parameters.Import("parameters.xml")
    parameters.Insert("parameters.xml")
    parameters.Export("parameters.xml")
    task_placement.setPropertyName("Placement")
    task_placement.setPlacement(placement)
    task_placement.setSelection([obj])
    task_placement.bindObject()
    task_placement.setPlacementAndBindObject(obj, "Placement")
    task_placement.setIgnoreTransactions(True)
    task_placement.showDefaultButtons(False)
    task_placement.clicked(1)
    task_placement.open()
    ui_loader.clearPluginPaths()
    ui_loader.addPluginPath("/tmp")
    ui_loader.setLanguageChangeEnabled(True)
    ui_loader.setWorkingDirectory("/tmp")
    assert_type(child_parameters, FreeCAD._ParameterGrp)
    assert_type(parameters.GetGroupName(), str)
    assert_type(parameters.GetGroups(), list[str])
    assert_type(parameters.HasGroup("Preferences"), bool)
    assert_type(parameters.RenameGroup("old", "new"), bool)
    assert_type(parameters.Manager(), FreeCAD._ParameterGrp | None)
    assert_type(parameters.Parent(), FreeCAD._ParameterGrp | None)
    assert_type(parameters.IsEmpty(), bool)
    assert_type(parameters.GetBool("flag", 0), bool)
    assert_type(parameters.GetBools(), list[str])
    assert_type(parameters.GetInt("number", 0), int)
    assert_type(parameters.GetInts(), list[str])
    assert_type(parameters.GetUnsigned("unsigned", 0), int)
    assert_type(parameters.GetUnsigneds(), list[str])
    assert_type(parameters.GetFloat("ratio", 0.0), float)
    assert_type(parameters.GetFloats(), list[str])
    assert_type(parameters.GetString("name", ""), str)
    assert_type(parameters.GetStrings(), list[str])
    assert_type(console_status, bool | None)
    assert_type(console_observers, list[str])
    assert_type(gui_up, int)
    assert_type(active_document, FreeCAD.Document | None)
    assert_type(part_feature, Part.Feature)
    assert_type(part_feature.Shape, Part.Shape)
    part_feature.Shape = shape
    assert_type(copied_object, FreeCAD.DocumentObject)
    assert_type(Console.PrintMessage("message"), None)
    assert_type(Console.GetStatus("Console", "Notification"), bool | None)
    assert_type(Console.GetObservers(), list[str])
    assert_type(parsed_quantity, Quantity)
    assert_type(schema, int)
    assert_type(schema_names, tuple[str, ...])
    assert_type(schema_description, str)
    assert_type(schema_translation, tuple[str, float, str])
    assert_type(quantity_number, str)
    assert_type(fixed_number, str)
    assert_type(Units.setSchema(schema), None)
    assert_type(Units.toNumber(1.0, "e"), str)
    assert_type(translated, str)
    assert_type(translated_with_disambiguation, str)
    assert_type(translate_noop, str)
    assert_type(translate_noop3, str)
    assert_type(tr_noop, str)
    assert_type(axis, Axis)
    assert_type(bound_box, BoundBox)
    assert_type(vector, FreeCAD.Vector)
    assert_type(matrix, FreeCAD.Matrix)
    assert_type(placement, FreeCAD.Placement)
    assert_type(rotation, FreeCAD.Rotation)
    assert_type(Qt.QT_TRANSLATE_NOOP_UTF8("Context", "Source"), str)
    assert_type(Qt.QT_TR_NOOP_UTF8("Source"), str)
    assert_type(Qt.installTranslator("translations.qm"), bool)
    assert_type(Qt.removeTranslators(), bool)
    assert_type(
        parameters.GetContents(),
        list[
            tuple[
                Literal["String", "Integer", "Float", "Boolean", "Unsigned Long"],
                str,
                str | int | float | bool,
            ]
        ]
        | None,
    )
    assert_type(control.activeDialog(), bool)
    assert_type(control.activeTaskDialog(), FreeCADGui._TaskDialog | None)
    assert_type(gui_document, FreeCADGui.Document | None)
    assert_type(edit_document, FreeCADGui.Document | None)
    assert_type(active_gui_document, FreeCADGui.Document | None)
    assert_type(gui_view, object | None)
    assert_type(gui_main_window, FreeCADGui._MainWindow)
    assert_type(resource_from_dialog, FreeCADGui._PyResource)
    assert_type(single_viewer, FreeCADGui._View3DInventor)
    assert_type(split_viewer, FreeCADGui._View3DInventor | FreeCADGui._AbstractSplitView)
    assert_type(material.Name, str)
    assert_type(path_command.toGCode(), str)
    assert_type(active_workbench, FreeCADGui.Workbench)
    assert_type(workbench_by_name, FreeCADGui.Workbench)
    assert_type(workbenches, dict[str, FreeCADGui.Workbench])
    assert_type(workbench, FreeCADGui.Workbench)
    assert_type(workbench.MenuText, str)
    assert_type(workbench.ToolTip, str)
    assert_type(workbench.Icon, object | None)
    assert_type(workbench.listToolbars(), list[str])
    assert_type(workbench.getToolbarItems(), dict[str, list[str]])
    assert_type(workbench.listCommandbars(), list[str])
    assert_type(workbench.listMenus(), list[str])
    assert_type(workbench.name(), str)
    assert_type(workbench.GetClassName(), str)
    assert_type(input_hint, FreeCADGui.InputHint)
    assert_type(
        input_hint.sequences,
        list[FreeCADGui.UserInput | tuple[FreeCADGui.UserInput, ...]],
    )
    assert_type(FreeCADGui.listCommands(), list[str])
    assert_type(FreeCADGui.isCommandActive("CommandName"), bool)
    assert_type(FreeCADGui.subgraphFromObject(obj), object | None)
    assert_type(FreeCADGui.getSoDBVersion(), str)
    assert_type(FreeCADGui.activateWorkbench("NoneWorkbench"), bool)
    assert_type(FreeCADGui.getIcon("example"), object | None)
    assert_type(FreeCADGui.isIconCached("example"), bool)
    assert_type(FreeCADGui.getLocale(), str)
    assert_type(FreeCADGui.supportedLocales(), dict[str, str])
    assert_type(FreeCADGui.activeView("Gui::View3DInventor"), object | None)
    assert_type(FreeCADGui.getDocument("Document"), FreeCADGui.Document)
    assert_type(FreeCADGui.doCommandEval("1 + 1"), Any)
    assert_type(FreeCADGui.createViewer(), FreeCADGui._View3DInventor)
    assert_type(FreeCADGui.createViewer(1, "Single"), FreeCADGui._View3DInventor)
    assert_type(FreeCADGui.getMarkerIndex("circle"), int)
    assert_type(FreeCADGui.listUserEditModes(), list[str])
    assert_type(FreeCADGui.getUserEditMode(), str)
    assert_type(FreeCADGui.setUserEditMode("Default"), bool)
    assert_type(FreeCADGui.reload("Document"), FreeCAD.Document | None)
    assert_type(main_window.getWindows(), list[FreeCADGui._MDIView])
    assert_type(main_window.getActiveWindow(), FreeCADGui._MDIView | None)
    assert_type(mdi_view.undoActions(), list[str])
    assert_type(mdi_view.sendMessage("ViewFit"), bool)
    assert_type(
        resource.value("objectName", "propertyName"),
        str | int | float | bool | list[str] | None,
    )
    assert_type(selection_filter.match(), bool)
    assert_type(selection_filter.getFilter(), str)
    assert_type(selection_filter.result(), list[tuple[FreeCADGui.SelectionObject, ...]])
    assert_type(preselection, FreeCADGui.SelectionObject)
    assert_type(selection, list[DocumentObject])
    assert_type(picked_selection, list[FreeCADGui.SelectionObject])
    assert_type(complete_selection, list[FreeCADGui.SelectionObject])
    assert_type(selection_ex, list[FreeCADGui.SelectionObject])
    assert_type(selection_object, FreeCADGui.SelectionObject)
    assert_type(stacked_selection, list[FreeCADGui.SelectionObject])
    assert_type(GuiSelection.countObjectsOfType("Part::Feature"), int)
    assert_type(GuiSelection.hasSelection(), bool)
    assert_type(GuiSelection.hasSubSelection(), bool)
    assert_type(task_dialog.getStandardButtons(), int)
    assert_type(task_dialog.isAllowedAlterDocument(), bool)
    assert_type(task_placement.accept(), bool)
    assert_type(task_placement.reject(), bool)
    assert_type(task_placement.isAllowedAlterDocument(), bool)
    assert_type(task_placement.isAllowedAlterView(), bool)
    assert_type(task_placement.isAllowedAlterSelection(), bool)
    assert_type(task_placement.getStandardButtons(), int)
    assert_type(ui_loader.load("dialog.ui"), object | None)
    assert_type(ui_loader.createWidget("Gui::InputField"), object | None)
    assert_type(ui_loader.availableWidgets(), list[str])
    assert_type(ui_loader.pluginPaths(), list[str])
    assert_type(ui_loader.errorString(), str)
    assert_type(ui_loader.isLanguageChangeEnabled(), bool)
    assert_type(ui_loader.workingDirectory(), str)
    assert_type(view.getCamera(), str)
    assert_type(view.getCameraType(), str)
    assert_type(view.listCameraTypes(), list[str])
    assert_type(view.getCursorPos(), tuple[int, int])
    assert_type(view.getSize(), tuple[int, int])
    assert_type(view.isAnimationEnabled(), bool)
    assert_type(view.hasAxisCross(), bool)
    assert_type(view.hasClippingPlane(), bool)
    assert_type(view.getCornerCrossSize(), int)
    assert_type(viewer.getFocalDistance(), float)
    assert_type(viewer.getPickRadius(), float)
    assert_type(viewer.isRedirectedToSceneGraph(), bool)
    assert_type(viewer.isEnabledNaviCube(), bool)
    protector.recompute()
    unit_test.getUnitTest()
    unit_test.clearErrorList()
    unit_test.insertError("failure", "details")
    unit_test.setUnitTest("TestApp.All")
    unit_test.setStatusText("Idle")
    unit_test.setProgressFraction(0.5)
    unit_test.setProgressFraction(1.0, "red")
    unit_test.errorDialog("title", "message")
    unit_test.setRunCount(1)
    unit_test.setFailCount(0)
    unit_test.setErrorCount(0)
    unit_test.setRemainCount(0)
    unit_test.updateGUI()
    unit_test.addUnitTest("TestApp.All")
    unit_test.clearUnitTests()
    assert_type(unit_test.getUnitTest(), str)
    sheet_view.selectedCells()
    page_view.getPage()
    shape_from_file = Part.read("example.step")
    solid = Part.makeBox(1.0, 2.0, 3.0)
    face = Part.makeFace([edge, wire])
    part_circle = Part.Circle()
    resolved = Part.getShape(obj, "Shape", matrix, False, True, 0)
    circle.copy()
    hlr.vCompound(shape)
    assert_type(part_circle.Center, FreeCAD.Vector)
    assert_type(part_circle.Axis, FreeCAD.Vector)
    assert_type(vector + vector, Vector)
    assert_type(vector - vector, Vector)
    assert_type(vector * 2.0, Vector)
    assert_type(vector * vector, float)
    assert_type(2.0 * vector, Vector)
    assert_type(vector / 2.0, Vector)
    assert_type(placement * vector, Vector)
    assert_type(placement * rotation, Placement)
    assert_type(placement * placement, Placement)
    assert_type(placement * matrix, Matrix)
    assert_type(rotation * vector, Vector)
    assert_type(rotation * placement, Placement)
    assert_type(rotation * rotation, Rotation)
    assert_type(rotation * matrix, Matrix)
    assert_type(Part.Precision, type[Precision])
    reveal_type(document)
    reveal_type(documents)
    reveal_type(transaction)
    reveal_type(parameters)
    reveal_type(child_parameters)
    reveal_type(progress)
    reveal_type(control)
    reveal_type(task_placement)
    reveal_type(ui_loader)
    reveal_type(main_window)
    reveal_type(mdi_view)
    reveal_type(task_dialog)
    reveal_type(view)
    reveal_type(viewer)
    reveal_type(resource)
    reveal_type(selection_filter)
    reveal_type(selection_object)
    reveal_type(protector)
    reveal_type(unit_test)
    reveal_type(sheet_view)
    reveal_type(page_view)
    reveal_type(shape_from_file)
    reveal_type(solid)
    reveal_type(face)
    reveal_type(resolved)
    reveal_type(vector)
