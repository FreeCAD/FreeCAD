// SPDX-License-Identifier: LGPL-2.1-or-later

#include <map>

#include <QAction>
#include <QMenu>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentPy.h>
#include <Base/Interpreter.h>

#include "Application.h"
#include "LayerWidget.h"
#include "LayerWidgetPy.h"
#include "PythonWrapper.h"
#include "ViewProviderDocumentObject.h"

using namespace Gui;

namespace
{
const char* DefaultsPath = "User parameter:BaseApp/Preferences/General/LayerDefaults";

Base::Color colorFromPython(const Py::Object& value, const Base::Color& fallback)
{
    if (value.isString()) {
        Base::Color color;
        return color.fromHexString(Py::String(value).as_std_string()) ? color : fallback;
    }
    if (value.isSequence()) {
        Py::Sequence sequence(value);
        if (sequence.size() >= 3) {
            return Base::Color(
                static_cast<float>(Py::Float(sequence[0])),
                static_cast<float>(Py::Float(sequence[1])),
                static_cast<float>(Py::Float(sequence[2]))
            );
        }
    }
    return fallback;
}

Py::Object colorToPython(const Base::Color& color)
{
    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(color.r));
    tuple.setItem(1, Py::Float(color.g));
    tuple.setItem(2, Py::Float(color.b));
    return tuple;
}

QVariant variantFromPython(const Py::Object& value)
{
    if (PyBool_Check(value.ptr())) {
        return value.isTrue();
    }
    if (PyLong_Check(value.ptr())) {
        return static_cast<int>(Py::Long(value));
    }
    if (PyFloat_Check(value.ptr())) {
        return static_cast<double>(Py::Float(value));
    }
    if (value.isString()) {
        return QString::fromStdString(Py::String(value).as_std_string());
    }
    return {};
}

Py::Object variantToPython(const QVariant& value)
{
    switch (value.typeId()) {
        case QMetaType::Bool:
            return Py::Boolean(value.toBool());
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
            return Py::Long(value.toLongLong());
        case QMetaType::Double:
            return Py::Float(value.toDouble());
        case QMetaType::QString:
            return Py::String(value.toString().toStdString());
        default:
            return Py::None();
    }
}

template<typename T>
T get(const Py::Dict& dict, const char* key, T fallback)
{
    if (!dict.hasKey(key)) {
        return fallback;
    }
    Py::Object value(dict.getItem(key));
    if constexpr (std::is_same_v<T, bool>) {
        return value.isTrue();
    }
    else if constexpr (std::is_same_v<T, double>) {
        return static_cast<double>(Py::Float(value));
    }
    else if constexpr (std::is_same_v<T, unsigned int>) {
        return static_cast<unsigned int>(Py::Long(value).as_unsigned_long());
    }
    else {
        return QString::fromStdString(Py::String(value).as_std_string());
    }
}

/// Forwards the model to a Python adapter object. Queries report Python errors and fall
/// back to nothing; changes raise them so that the widget aborts the transaction.
class PythonLayerModel: public LayerModel
{
public:
    explicit PythonLayerModel(const Py::Object& adapter)
        : adapter(adapter)
    {
        if (auto* doc = document()) {
            auto changed = [this](auto&&...) {
                signalChanged();
            };
            connections.emplace_back(doc->signalNewObject.connect(changed));
            connections.emplace_back(doc->signalDeletedObject.connect(changed));
            connections.emplace_back(doc->signalChangedObject.connect(changed));
            connections.emplace_back(doc->signalUndo.connect(changed));
            connections.emplace_back(doc->signalRedo.connect(changed));
            // View properties (colors, line styles) change without touching the document.
            connections.emplace_back(
                Application::Instance->signalChangedObject.connect(
                    [this, doc](const ViewProvider& view, const App::Property&) {
                        const auto* object = dynamic_cast<const ViewProviderDocumentObject*>(&view);
                        if (object && object->getObject()
                            && object->getObject()->getDocument() == doc) {
                            signalChanged();
                        }
                    }
                )
            );
        }
    }
    ~PythonLayerModel() override
    {
        Base::PyGILStateLocker lock;
        ids.clear();
        adapter = Py::None();
    }

    std::vector<LayerInfo> layers() const override
    {
        std::vector<LayerInfo> result;
        Base::PyGILStateLocker lock;
        try {
            ids.clear();
            const auto fallback = fallbackStyle();
            Py::Sequence list(call("layers"));
            for (const auto& entry : list) {
                Py::Dict dict(entry);
                LayerInfo layer;
                Py::Object id(dict.getItem("id"));
                layer.id = id.isString() ? Py::String(id).as_std_string() : id.str().as_std_string();
                ids[layer.id] = id;
                layer.name = get<QString>(dict, "name", QString::fromStdString(layer.id)).toStdString();
                layer.style.color = dict.hasKey("color")
                    ? colorFromPython(dict.getItem("color"), fallback.color)
                    : fallback.color;
                layer.style.pattern = get(dict, "pattern", fallback.pattern);
                layer.style.lineWidth = get(dict, "lineWidth", fallback.lineWidth);
                layer.visible = get(dict, "visible", true);
                layer.locked = get(dict, "locked", false);
                layer.active = get(dict, "active", false);
                layer.removable = get(dict, "removable", true);
                result.push_back(std::move(layer));
            }
        }
        catch (Py::Exception&) {
            Base::PyException().reportException();
        }
        return result;
    }

    bool hasFeature(Feature feature) const override
    {
        switch (feature) {
            case Feature::Add:
                return has("addLayer");
            case Feature::Remove:
                return has("removeLayer");
            case Feature::Rename:
                return has("renameLayer");
            case Feature::Reorder:
                return has("reorderLayers");
            case Feature::Activate:
                return has("setActiveLayer");
            case Feature::Lock:
                return has("setLayerLocked");
            case Feature::Style:
                return has("setLayerColor") && has("setLayerPattern") && has("setLayerLineWidth");
        }
        return false;
    }

    App::Document* document() const override
    {
        if (!has("document")) {
            return nullptr;
        }
        Base::PyGILStateLocker lock;
        try {
            Py::Object doc(call("document"));
            if (PyObject_TypeCheck(doc.ptr(), &App::DocumentPy::Type)) {
                return static_cast<App::DocumentPy*>(doc.ptr())->getDocumentPtr();
            }
        }
        catch (Py::Exception&) {
            Base::PyException().reportException();
        }
        return nullptr;
    }

    void addLayer(const std::string& name) override
    {
        Base::PyGILStateLocker lock;
        change("addLayer", Py::String(name));
    }
    void renameLayer(const std::string& id, const std::string& name) override
    {
        Base::PyGILStateLocker lock;
        change("renameLayer", pyId(id), Py::String(name));
    }
    bool prepareRemoveLayer(const std::string& id, QWidget*) override
    {
        if (!has("prepareRemoveLayer")) {
            return true;
        }
        Base::PyGILStateLocker lock;
        try {
            return call("prepareRemoveLayer", pyId(id)).isTrue();
        }
        catch (Py::Exception&) {
            Base::PyException().reportException();
            return false;
        }
    }
    void removeLayer(const std::string& id) override
    {
        Base::PyGILStateLocker lock;
        change("removeLayer", pyId(id));
    }
    void setActiveLayer(const std::string& id) override
    {
        Base::PyGILStateLocker lock;
        change("setActiveLayer", pyId(id));
    }
    void setLayerVisible(const std::string& id, bool visible) override
    {
        Base::PyGILStateLocker lock;
        change("setLayerVisible", pyId(id), Py::Boolean(visible));
    }
    void setLayerLocked(const std::string& id, bool locked) override
    {
        Base::PyGILStateLocker lock;
        change("setLayerLocked", pyId(id), Py::Boolean(locked));
    }
    void setLayerColor(const std::string& id, const Base::Color& color) override
    {
        Base::PyGILStateLocker lock;
        change("setLayerColor", pyId(id), colorToPython(color));
    }
    void setLayerPattern(const std::string& id, unsigned int pattern) override
    {
        Base::PyGILStateLocker lock;
        change("setLayerPattern", pyId(id), Py::Long(static_cast<unsigned long>(pattern)));
    }
    void setLayerLineWidth(const std::string& id, double width) override
    {
        Base::PyGILStateLocker lock;
        change("setLayerLineWidth", pyId(id), Py::Float(width));
    }
    void reorderLayers(const std::vector<std::string>& order) override
    {
        Base::PyGILStateLocker lock;
        Py::List list;
        for (const auto& id : order) {
            list.append(pyId(id));
        }
        change("reorderLayers", list);
    }

    std::vector<LayerSetting> extraSettings() const override
    {
        std::vector<LayerSetting> result;
        if (!has("extraSettings")) {
            return result;
        }
        Base::PyGILStateLocker lock;
        try {
            Py::Sequence list(call("extraSettings"));
            for (const auto& entry : list) {
                Py::Dict dict(entry);
                LayerSetting setting;
                setting.key = Py::String(dict.getItem("key")).as_std_string();
                const auto type = get<QString>(dict, "type", QStringLiteral("bool"));
                setting.type = type == QLatin1String("float") ? LayerSetting::Type::Double
                    : type == QLatin1String("choice")         ? LayerSetting::Type::Choice
                                                              : LayerSetting::Type::Bool;
                setting.label = get<QString>(dict, "label", QString::fromStdString(setting.key));
                setting.toolTip = get<QString>(dict, "toolTip", {});
                setting.defaultsLabel = get<QString>(dict, "defaultsLabel", {});
                setting.defaultsToolTip = get<QString>(dict, "defaultsToolTip", {});
                if (dict.hasKey("default")) {
                    setting.defaultValue = variantFromPython(dict.getItem("default"));
                }
                setting.minimum = get(dict, "minimum", setting.minimum);
                setting.maximum = get(dict, "maximum", setting.maximum);
                setting.suffix = get<QString>(dict, "suffix", {});
                if (dict.hasKey("choices")) {
                    for (const auto& choice : Py::Sequence(dict.getItem("choices"))) {
                        setting.choices.append(
                            QString::fromStdString(Py::String(choice).as_std_string())
                        );
                    }
                }
                setting.dependsOn = get<QString>(dict, "dependsOn", {}).toStdString();
                result.push_back(std::move(setting));
            }
        }
        catch (Py::Exception&) {
            Base::PyException().reportException();
        }
        return result;
    }
    QVariant setting(const std::string& id, const std::string& key) const override
    {
        if (!has("setting")) {
            return {};
        }
        Base::PyGILStateLocker lock;
        try {
            return variantFromPython(call("setting", pyId(id), Py::String(key)));
        }
        catch (Py::Exception&) {
            Base::PyException().reportException();
        }
        return {};
    }
    void setSetting(const std::string& id, const std::string& key, const QVariant& value) override
    {
        Base::PyGILStateLocker lock;
        change("setSetting", pyId(id), Py::String(key), variantToPython(value));
    }

    void populateContextMenu(QMenu* menu, const std::string& id) override
    {
        if (!has("contextMenu")) {
            return;
        }
        Base::PyGILStateLocker lock;
        try {
            Py::Object entries(call("contextMenu", pyId(id)));
            if (entries.isNone()) {
                return;
            }
            for (const auto& entry : Py::Sequence(entries)) {
                if (Py::Object(entry).isNone()) {
                    menu->addSeparator();
                    continue;
                }
                Py::Sequence pair(entry);
                auto* action = menu->addAction(
                    QString::fromStdString(Py::String(pair[0]).as_std_string())
                );
                auto callback = std::make_shared<Py::Object>(pair[1]);
                QObject::connect(action, &QAction::triggered, action, [callback]() {
                    Base::PyGILStateLocker lock;
                    try {
                        Py::Callable(*callback).apply(Py::Tuple());
                    }
                    catch (Py::Exception&) {
                        Base::PyException().reportException();
                    }
                });
                // The callable must be released while holding the GIL.
                QObject::connect(action, &QObject::destroyed, [callback]() {
                    Base::PyGILStateLocker lock;
                    *callback = Py::None();
                });
            }
        }
        catch (Py::Exception&) {
            Base::PyException().reportException();
        }
    }

    ParameterGrp::handle defaultsGroup() const override
    {
        std::string path = DefaultsPath;
        if (has("defaultsGroup")) {
            Base::PyGILStateLocker lock;
            try {
                path = Py::String(call("defaultsGroup")).as_std_string();
            }
            catch (Py::Exception&) {
                Base::PyException().reportException();
            }
        }
        return App::GetApplication().GetParameterGroupByPath(path.c_str());
    }

    LayerStyle fallbackStyle() const override
    {
        auto style = LayerModel::fallbackStyle();
        if (!has("fallbackStyle")) {
            return style;
        }
        Base::PyGILStateLocker lock;
        try {
            Py::Dict dict(call("fallbackStyle"));
            if (dict.hasKey("color")) {
                style.color = colorFromPython(dict.getItem("color"), style.color);
            }
            style.pattern = get(dict, "pattern", style.pattern);
            style.lineWidth = get(dict, "lineWidth", style.lineWidth);
        }
        catch (Py::Exception&) {
            Base::PyException().reportException();
        }
        return style;
    }

private:
    bool has(const char* name) const
    {
        Base::PyGILStateLocker lock;
        return adapter.hasAttr(name);
    }

    template<typename... Args>
    Py::Object call(const char* name, const Args&... args) const
    {
        Py::Tuple tuple(static_cast<int>(sizeof...(Args)));
        int index = 0;
        (tuple.setItem(index++, args), ...);
        return Py::Callable(adapter.getAttr(name)).apply(tuple);
    }

    template<typename... Args>
    void change(const char* name, const Args&... args)
    {
        Base::PyGILStateLocker lock;
        try {
            call(name, args...);
        }
        catch (Py::Exception&) {
            throw Base::PyException();
        }
    }

    Py::Object pyId(const std::string& id) const
    {
        auto it = ids.find(id);
        return it == ids.end() ? Py::Object(Py::String(id)) : it->second;
    }

    Py::Object adapter;
    /// The adapter's own id objects, so that it gets back what it gave, e.g. an int.
    mutable std::map<std::string, Py::Object> ids;
    std::vector<fastsignals::scoped_connection> connections;
};
}  // namespace

// ----------------------------------------------------------------------------

void LayerWidgetPy::init_type()
{
    behaviors().name("LayerWidget");
    behaviors().doc(
        "LayerWidget(adapter)\n"
        "A layer list driven by a Python adapter object. The widget is available as 'form'.\n\n"
        "Required adapter methods:\n"
        "  layers() -> list of dict: id, name, visible and optionally color ((r, g, b) or\n"
        "      '#rrggbb'), pattern (16-bit int), lineWidth, locked, active, removable\n"
        "  setLayerVisible(id, bool)\n"
        "Optional methods, each enabling a feature when present:\n"
        "  document() -> App.Document for undo transactions and automatic refresh\n"
        "  addLayer(name), removeLayer(id), prepareRemoveLayer(id) -> bool,\n"
        "  renameLayer(id, name), reorderLayers([id]), setActiveLayer(id),\n"
        "  setLayerLocked(id, bool),\n"
        "  setLayerColor(id, (r, g, b)), setLayerPattern(id, int), setLayerLineWidth(id, float),\n"
        "  extraSettings() -> list of dict: key, type ('bool', 'float', 'choice'), label,\n"
        "      toolTip, defaultsLabel, defaultsToolTip, default, minimum, maximum, suffix,\n"
        "      choices, dependsOn\n"
        "  setting(id, key), setSetting(id, key, value),\n"
        "  contextMenu(id) -> list of (label, callable) or None for a separator,\n"
        "  defaultsGroup() -> parameter group path for the defaults of new layers,\n"
        "  fallbackStyle() -> dict: color, pattern, lineWidth"
    );
    behaviors().set_tp_new(PyMake);
    behaviors().supportRepr();
    behaviors().supportGetattr();
    add_varargs_method(
        "refresh",
        &LayerWidgetPy::refresh,
        "refresh()\nReloads the layers from the adapter."
    );
}

PyObject* LayerWidgetPy::PyMake(struct _typeobject*, PyObject* args, PyObject*)
{
    PyObject* adapter;
    if (!PyArg_ParseTuple(args, "O", &adapter)) {
        return nullptr;
    }
    try {
        return new LayerWidgetPy(Py::Object(adapter));
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}

LayerWidgetPy::LayerWidgetPy(const Py::Object& adapter)
    : widget(new LayerWidget(std::make_unique<PythonLayerModel>(adapter)))
{}

LayerWidgetPy::~LayerWidgetPy()
{
    // Once shown in a panel, the widget belongs to it.
    if (widget && !widget->parent()) {
        widget->deleteLater();
    }
}

Py::Object LayerWidgetPy::repr()
{
    return Py::String("LayerWidget");
}

Py::Object LayerWidgetPy::getattr(const char* name)
{
    if (strcmp(name, "form") == 0) {
        if (!widget) {
            return Py::None();
        }
        PythonWrapper wrap;
        wrap.loadWidgetsModule();
        return wrap.fromQWidget(widget, "QWidget");
    }
    return BaseType::getattr(name);
}

Py::Object LayerWidgetPy::refresh(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    if (!widget) {
        throw Py::RuntimeError("The layer widget was deleted");
    }
    widget->refresh();
    return Py::None();
}
