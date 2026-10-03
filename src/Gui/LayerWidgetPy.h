// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QPointer>
#include <CXX/Extensions.hxx>

namespace Gui
{
class LayerWidget;

/** Python access to the layer widget: FreeCADGui.LayerWidget(adapter).
 *
 * The adapter is any Python object implementing the LayerModel methods (layers(),
 * setLayerVisible(), ...); missing optional methods turn the matching feature off. The
 * widget is available as `form`, a PySide widget ready for a task panel or dialog.
 */
class LayerWidgetPy: public Py::PythonExtension<LayerWidgetPy>
{
public:
    using BaseType = Py::PythonExtension<LayerWidgetPy>;
    static void init_type();

    explicit LayerWidgetPy(const Py::Object& adapter);
    ~LayerWidgetPy() override;

    Py::Object repr() override;
    Py::Object getattr(const char* name) override;

    Py::Object refresh(const Py::Tuple&);

private:
    static PyObject* PyMake(struct _typeobject*, PyObject*, PyObject*);

    QPointer<LayerWidget> widget;
};

}  // namespace Gui
