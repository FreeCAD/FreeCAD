// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AssemblyDocumentObserver.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <fastsignals/signal.h>

namespace CadX
{

class AssemblyDocumentObserver::FreeCadObserver final : public App::DocumentObserver
{
public:
    explicit FreeCadObserver(AssemblyDocumentObserver& owner)
        : _owner(owner)
    {
        auto& application = App::GetApplication();
        _changedDocument = application.signalChangedDocument.connect(
            [this](const App::Document& document, const App::Property&) {
                documentChanged(document, "FreeCAD document property changed");
            });
        _createdObject = application.signalNewObject.connect(
            [this](const App::DocumentObject& object) { objectChanged(object, "object created"); });
        _deletedObject = application.signalDeletedObject.connect(
            [this](const App::DocumentObject& object) { objectChanged(object, "object deleted"); });
        _changedObject = application.signalChangedObject.connect(
            [this](const App::DocumentObject& object, const App::Property&) {
                objectChanged(object, "FreeCAD object property changed");
            });
        _recomputedObject = application.signalObjectRecomputed.connect(
            [this](const App::DocumentObject& object) {
                objectChanged(object, "FreeCAD object recomputed");
            });
        _recomputedDocument = application.signalRecomputed.connect(
            [this](const App::Document& document) {
                documentChanged(document, "FreeCAD document recomputed");
            });
    }

    ~FreeCadObserver() override
    {
        // Disconnect aggregate signals before the base observer disconnects
        // its application signals and before the owning GraphStore is gone.
        _changedDocument.disconnect();
        _createdObject.disconnect();
        _deletedObject.disconnect();
        _changedObject.disconnect();
        _recomputedObject.disconnect();
        _recomputedDocument.disconnect();
    }

private:
    void documentChanged(const App::Document& document, const std::string& reason)
    {
        const auto uid = document.Uid.getValueStr();
        if (!uid.empty()) {
            _owner.sourceDocumentChanged(uid, reason);
        }
    }

    void objectChanged(const App::DocumentObject& object, const std::string& reason)
    {
        auto* document = object.getDocument();
        if (document) {
            documentChanged(*document, reason);
        }
    }

    void slotDeletedDocument(const App::Document& document) override
    {
        const auto uid = document.Uid.getValueStr();
        if (!uid.empty()) {
            _owner.sourceDocumentChanged(uid, "FreeCAD document closed");
            _owner._store.removeDocument(uid);
        }
    }

    AssemblyDocumentObserver& _owner;
    fastsignals::connection _changedDocument;
    fastsignals::connection _createdObject;
    fastsignals::connection _deletedObject;
    fastsignals::connection _changedObject;
    fastsignals::connection _recomputedObject;
    fastsignals::connection _recomputedDocument;
};

AssemblyDocumentObserver::AssemblyDocumentObserver(GraphStore& store, bool connectToFreeCad)
    : _store(store)
{
    if (connectToFreeCad) {
        _freeCadObserver = std::make_unique<FreeCadObserver>(*this);
    }
}

AssemblyDocumentObserver::~AssemblyDocumentObserver() = default;

}  // namespace CadX
